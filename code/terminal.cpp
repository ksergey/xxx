// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// Native backend for xterm compatible terminals: termios raw mode, alternate screen,
// SGR mouse, bracketed paste, synchronized output. No terminfo, no third party code.

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "ansi_input.h"
#include "ansi_screen.h"
#include "base64.h"
#include "im_backend.h"
#include "terminal.h"

namespace xxx {

namespace {

constexpr auto enter_sequence = std::string_view("\x1b[?1049h" // alternate screen
                                                 "\x1b[?25l"   // hide cursor
                                                 "\x1b[?1000h" // mouse buttons
                                                 "\x1b[?1003h" // mouse motion
                                                 "\x1b[?1006h" // SGR mouse coordinates
                                                 "\x1b[?2004h" // bracketed paste
);
constexpr auto leave_sequence = std::string_view("\x1b[?2026l" // end synchronized output (if interrupted)
                                                 "\x1b[0m"
                                                 "\x1b[?2004l"
                                                 "\x1b[?1006l"
                                                 "\x1b[?1003l"
                                                 "\x1b[?1000l"
                                                 "\x1b[?25h"
                                                 "\x1b[?1049l");

// after a lone ESC wait this long for the rest of a sequence before treating it as the Esc key.
// ESCDELAY (milliseconds, the ncurses convention) overrides it, e.g. for slow ssh links.
constexpr auto default_esc_timeout_ms = 25;

// what colors the terminal shows, from the usual conventions:
// NO_COLOR (no-color.org) - none; COLORTERM=truecolor|24bit - 24-bit; TERM *256color* - 256; otherwise 16
[[nodiscard]] auto color_mode_from_env() noexcept -> ansi_screen::color_mode {
    auto const env = [](char const* name) {
        auto const* v = std::getenv(name);
        return std::string_view(v ? v : "");
    };
    if (!env("NO_COLOR").empty()) {
        return ansi_screen::color_mode::none;
    }
    if (auto const colorterm = env("COLORTERM"); colorterm == "truecolor" || colorterm == "24bit") {
        return ansi_screen::color_mode::truecolor;
    }
    auto const term = env("TERM");
    if (term.find("direct") != std::string_view::npos || term.find("truecolor") != std::string_view::npos) {
        return ansi_screen::color_mode::truecolor;
    }
    if (term.find("256") != std::string_view::npos) {
        return ansi_screen::color_mode::palette256;
    }
    return ansi_screen::color_mode::ansi16;
}

[[nodiscard]] auto esc_timeout_from_env() noexcept -> int {
    if (auto const* const value = std::getenv("ESCDELAY")) {
        auto ms = 0;
        auto const end = value + std::strlen(value);
        if (auto const [ptr, ec] = std::from_chars(value, end, ms); ec == std::errc() && ptr == end) {
            return std::clamp(ms, 0, 5000);
        }
    }
    return default_esc_timeout_ms;
}

// terminal state for signal handlers: they restore the terminal before the process dies
struct tty_state {
    int fd = -1;
    termios saved{};
    int resize_pipe_write = -1;
    bool active = false;
};
tty_state g_tty;

constexpr int fatal_signals[] = {SIGTERM, SIGHUP, SIGINT, SIGQUIT, SIGABRT, SIGSEGV, SIGBUS, SIGFPE, SIGILL};
struct sigaction g_old_actions[std::size(fatal_signals)];
struct sigaction g_old_winch;

void write_all(int fd, char const* data, std::size_t size) noexcept {
    while (size > 0) {
        auto const n = ::write(fd, data, size);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return; // nothing sensible to do: terminal is gone
        }
        data += n;
        size -= std::size_t(n);
    }
}

// async-signal-safe: write(2) and tcsetattr(3) only
void restore_terminal() noexcept {
    if (!g_tty.active) {
        return;
    }
    g_tty.active = false;
    write_all(g_tty.fd, leave_sequence.data(), leave_sequence.size());
    ::tcsetattr(g_tty.fd, TCSAFLUSH, &g_tty.saved);
}

void on_fatal_signal(int sig) {
    restore_terminal();
    // die the way the signal intended (core dump, exit status)
    ::signal(sig, SIG_DFL);
    ::raise(sig);
}

void on_winch(int) {
    auto const saved_errno = errno;
    char const byte = 1;
    [[maybe_unused]] auto const n = ::write(g_tty.resize_pipe_write, &byte, 1);
    errno = saved_errno;
}

[[nodiscard]] auto system_error(char const* what) -> std::system_error {
    return std::system_error(errno, std::generic_category(), what);
}

} // namespace

struct terminal::impl {
    terminal::options opts;
    int fd = -1;
    bool owns_fd = true;
    std::string inflight; // handed to opts.write, not done yet
    std::size_t inflight_done = 0;
    bool writing = false;
    bool frame_held = false;
    int notify_pipe[2] = {-1, -1};
    int esc_timeout_ms = esc_timeout_from_env();
    ansi_input_parser parser;
    ansi_screen screen;
    std::string out;
    std::deque<im_event> queue;
    std::vector<im_event> scratch;
    std::chrono::steady_clock::time_point esc_deadline;
    bool notified = false;

    explicit impl(terminal::options o) : opts(std::move(o)) {
        if (g_tty.active) {
            throw std::runtime_error("terminal is already in use");
        }
        if (opts.tty_fd >= 0) {
            fd = opts.tty_fd;
            owns_fd = false;
        } else {
            // /dev/tty works even when stdin / stdout are redirected
            fd = ::open("/dev/tty", O_RDWR | O_NOCTTY | O_CLOEXEC);
            if (fd < 0) {
                throw system_error("open /dev/tty");
            }
        }
        auto const close_fd = [this] {
            if (owns_fd) {
                ::close(fd);
            }
        };
        if (::tcgetattr(fd, &g_tty.saved) != 0) {
            auto const error = system_error("tcgetattr");
            close_fd();
            throw error;
        }
        // pipe + fcntl instead of Linux-only pipe2: macOS and BSDs have no pipe2
        if (::pipe(notify_pipe) != 0) {
            auto const error = system_error("pipe");
            close_fd();
            throw error;
        }
        for (auto const p : notify_pipe) {
            ::fcntl(p, F_SETFD, FD_CLOEXEC);
            ::fcntl(p, F_SETFL, ::fcntl(p, F_GETFL) | O_NONBLOCK);
        }

        auto raw = g_tty.saved;
        raw.c_iflag &= ~tcflag_t(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
        raw.c_oflag &= ~tcflag_t(OPOST);
        raw.c_lflag &= ~tcflag_t(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        raw.c_cflag &= ~tcflag_t(CSIZE | PARENB);
        raw.c_cflag |= CS8;
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        ::tcsetattr(fd, TCSAFLUSH, &raw);

        g_tty.fd = fd;
        g_tty.resize_pipe_write = notify_pipe[1];
        g_tty.active = true;

        struct sigaction action{};
        sigemptyset(&action.sa_mask);
        action.sa_handler = on_fatal_signal;
        for (std::size_t i = 0; i < std::size(fatal_signals); ++i) {
            ::sigaction(fatal_signals[i], &action, &g_old_actions[i]);
        }
        if (opts.handle_resize_signal) {
            action.sa_handler = on_winch;
            action.sa_flags = SA_RESTART;
            ::sigaction(SIGWINCH, &action, &g_old_winch);
        }

        write_all(fd, enter_sequence.data(), enter_sequence.size());
        screen.set_color_mode(color_mode_from_env());
        update_size();
    }

    ~impl() {
        restore_terminal();
        for (std::size_t i = 0; i < std::size(fatal_signals); ++i) {
            ::sigaction(fatal_signals[i], &g_old_actions[i], nullptr);
        }
        if (opts.handle_resize_signal) {
            ::sigaction(SIGWINCH, &g_old_winch, nullptr);
        }
        g_tty = {};
        ::close(notify_pipe[0]);
        ::close(notify_pipe[1]);
        if (owns_fd) {
            ::close(fd);
        }
    }

    // true if the size changed
    auto update_size() -> bool {
        winsize ws{};
        if (::ioctl(fd, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0 || ws.ws_row == 0) {
            ws.ws_col = 80, ws.ws_row = 24;
        }
        if (screen.size() == im_vec2(ws.ws_col, ws.ws_row)) {
            return false;
        }
        screen.resize(im_vec2(ws.ws_col, ws.ws_row)); // full redraw on next present
        return true;
    }

    // parsed events -> queue
    void enqueue_parsed() {
        queue.insert(queue.end(), scratch.begin(), scratch.end());
        scratch.clear();
    }

    // resize / wake_up through the self-pipe
    auto read_notifications() -> bool {
        auto got = false;
        char drain[64];
        while (::read(notify_pipe[0], drain, sizeof(drain)) > 0) {
            got = true;
        }
        if (got) {
            notify_resize();
        }
        return got;
    }

    void notify_resize() {
        notified = true;
        if (update_size()) {
            queue.push_back({.kind = im_event::type::resize, .pos = screen.size()});
        }
    }

    void feed(std::string_view bytes) {
        auto const was_pending = parser.pending();
        parser.feed(bytes, scratch);
        enqueue_parsed();
        if (parser.pending() && !was_pending) {
            esc_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(esc_timeout_ms);
        }
    }

    auto read_available() -> bool {
        auto const queued = queue.size();
        auto const got_notification = read_notifications();

        // input: read while poll says there is something, the descriptor itself stays blocking
        while (true) {
            pollfd p = {.fd = fd, .events = POLLIN, .revents = 0};
            if (::poll(&p, 1, 0) <= 0) {
                break;
            }
            if (p.revents & POLLIN) {
                char bytes[4096];
                auto const n = ::read(fd, bytes, sizeof(bytes));
                if (n < 0 && errno != EINTR && errno != EAGAIN) {
                    throw system_error("read terminal");
                }
                if (n <= 0) {
                    break;
                }
                feed(std::string_view(bytes, std::size_t(n)));
            } else if (p.revents & (POLLHUP | POLLERR | POLLNVAL)) {
                throw std::runtime_error("terminal closed");
            } else {
                break;
            }
        }
        return queue.size() != queued || got_notification;
    }

    // frame output: synchronous, or handed to opts.write one at a time
    void present() {
        if (writing) {
            frame_held = true; // the back buffer keeps the latest frame, it goes after the write in flight
            return;
        }
        screen.present(out);
        send();
    }

    void send() {
        if (out.empty()) {
            return;
        }
        if (!opts.write) {
            write_all(fd, out.data(), out.size());
            out.clear(); // keeps capacity
            return;
        }
        inflight.swap(out);
        out.clear();
        inflight_done = 0;
        writing = true;
        opts.write(inflight);
    }

    void write_done(std::size_t written) {
        if (!writing) {
            return;
        }
        inflight_done = std::min(inflight.size(), inflight_done + written);
        if (inflight_done < inflight.size()) {
            opts.write(std::string_view(inflight).substr(inflight_done)); // partial write: the rest
            return;
        }
        writing = false;
        inflight.clear();
        if (std::exchange(frame_held, false)) {
            screen.present(out);
        }
        send(); // the held frame and / or bytes queued meanwhile (clipboard)
    }

    [[nodiscard]] auto timeout_ms() const noexcept -> int {
        if (!parser.pending()) {
            return -1;
        }
        auto const left =
            std::chrono::duration_cast<std::chrono::milliseconds>(esc_deadline - std::chrono::steady_clock::now());
        return int(std::max<std::int64_t>(0, left.count()));
    }

    auto check_timeout() -> bool {
        if (!parser.pending() || std::chrono::steady_clock::now() < esc_deadline) {
            return false;
        }
        auto const queued = queue.size();
        parser.flush(scratch);
        enqueue_parsed();
        return queue.size() != queued;
    }

    void wake_up() noexcept {
        // same self-pipe as SIGWINCH
        char const byte = 2;
        [[maybe_unused]] auto const n = ::write(notify_pipe[1], &byte, 1);
    }
};

terminal::terminal() : terminal(options{}) {}
terminal::terminal(options opts) : impl_(std::make_unique<impl>(std::move(opts))) {}
terminal::~terminal() = default;

auto terminal::input_fd() const noexcept -> int {
    return impl_->fd;
}
auto terminal::notify_fd() const noexcept -> int {
    return impl_->notify_pipe[0];
}
auto terminal::timeout_ms() const noexcept -> int {
    return impl_->timeout_ms();
}
auto terminal::read_available() -> bool {
    return impl_->read_available();
}
auto terminal::check_timeout() -> bool {
    return impl_->check_timeout();
}
void terminal::feed(std::string_view bytes) {
    impl_->feed(bytes);
}
auto terminal::read_notifications() -> bool {
    return impl_->read_notifications();
}
void terminal::notify_resize() {
    impl_->notify_resize();
}
void terminal::write_done(std::size_t written) {
    impl_->write_done(written);
}
auto terminal::next_event(im_event& event) -> bool {
    if (impl_->queue.empty()) {
        return false;
    }
    event = impl_->queue.front();
    impl_->queue.pop_front();
    return true;
}
auto terminal::size() const noexcept -> im_vec2 {
    return impl_->screen.size();
}
void terminal::wake_up() noexcept {
    impl_->wake_up();
}

struct terminal_access {
    static auto get(terminal& t) noexcept -> terminal::impl& {
        return *t.impl_;
    }
};

namespace {

// UI backend on a terminal: owns it and waits itself (init()), or uses an external one and never waits (init(term))
class terminal_backend final : public im_backend {
public:
    explicit terminal_backend(std::unique_ptr<terminal> owned)
        : owned_(std::move(owned)), term_(*owned_), waits_(true) {}
    explicit terminal_backend(terminal& external) : term_(external), waits_(false) {}

    [[nodiscard]] auto size() const -> im_vec2 override {
        return term_.size();
    }

    auto poll_events(im_input& input, std::chrono::milliseconds timeout) -> bool override {
        auto& t = terminal_access::get(term_);
        if (waits_) {
            // block only before the first event, then drain what is already there
            for (auto first = true;; first = false) {
                auto wait = first ? int(std::clamp<std::int64_t>(timeout.count(), -1, INT32_MAX)) : 0;
                if (auto const esc = t.timeout_ms(); esc >= 0) {
                    wait = (wait < 0) ? esc : std::min(wait, esc);
                }
                pollfd fds[2] = {{.fd = t.fd, .events = POLLIN, .revents = 0},
                    {.fd = t.notify_pipe[0], .events = POLLIN, .revents = 0}};
                auto const rc = ::poll(fds, 2, wait);
                if (rc < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    throw system_error("poll");
                }
                if (rc == 0) {
                    break;
                }
                t.read_available();
            }
        }
        t.check_timeout();

        auto got = std::exchange(t.notified, false);
        for (im_event e; term_.next_event(e);) {
            apply_event(e, input);
            got = true;
        }
        return got;
    }

    void clear(im_style const& style) override {
        terminal_access::get(term_).screen.clear(style);
    }

    void set_cell(int x, int y, std::uint32_t ch, im_style const& style) override {
        terminal_access::get(term_).screen.set_cell(x, y, ch, style);
    }

    void present() override {
        terminal_access::get(term_).present();
    }

    void set_clipboard(std::string_view text) override {
        // sent with the next frame, like any other output
        terminal_access::get(term_).out += osc52_sequence(text, std::getenv("TMUX") != nullptr);
    }

    void wake_up() override {
        term_.wake_up();
    }

private:
    std::unique_ptr<terminal> owned_;
    terminal& term_;
    bool waits_;
};

} // namespace

auto make_ansi_backend() -> std::unique_ptr<im_backend> {
    return std::make_unique<terminal_backend>(std::make_unique<terminal>());
}

auto make_terminal_backend(terminal& external) -> std::unique_ptr<im_backend> {
    return std::make_unique<terminal_backend>(external);
}

} // namespace xxx
