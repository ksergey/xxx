// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// Native backend for xterm compatible terminals: termios raw mode, alternate screen,
// SGR mouse, bracketed paste, synchronized output. No terminfo, no third party code.

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "ansi_input.h"
#include "ansi_screen.h"
#include "base64.h"
#include "im_backend.h"

namespace xxx {

namespace {

constexpr auto enter_sequence = std::string_view("\x1b[?1049h"  // alternate screen
                                                 "\x1b[?25l"    // hide cursor
                                                 "\x1b[?1000h"  // mouse buttons
                                                 "\x1b[?1003h"  // mouse motion
                                                 "\x1b[?1006h"  // SGR mouse coordinates
                                                 "\x1b[?2004h"  // bracketed paste
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

class im_backend_ansi final : public im_backend {
public:
  im_backend_ansi() {
    if (g_tty.active) {
      throw std::runtime_error("terminal is already in use by another backend");
    }
    // /dev/tty works even when stdin / stdout are redirected
    fd_ = ::open("/dev/tty", O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (fd_ < 0) {
      throw system_error("open /dev/tty");
    }
    if (::tcgetattr(fd_, &g_tty.saved) != 0) {
      auto const error = system_error("tcgetattr");
      ::close(fd_);
      throw error;
    }
    // pipe + fcntl instead of Linux-only pipe2: macOS and BSDs have no pipe2
    if (::pipe(resize_pipe_) != 0) {
      auto const error = system_error("pipe");
      ::close(fd_);
      throw error;
    }
    for (auto const fd : resize_pipe_) {
      ::fcntl(fd, F_SETFD, FD_CLOEXEC);
      ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
    }

    auto raw = g_tty.saved;
    raw.c_iflag &= ~tcflag_t(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    raw.c_oflag &= ~tcflag_t(OPOST);
    raw.c_lflag &= ~tcflag_t(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw.c_cflag &= ~tcflag_t(CSIZE | PARENB);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    ::tcsetattr(fd_, TCSAFLUSH, &raw);

    g_tty.fd = fd_;
    g_tty.resize_pipe_write = resize_pipe_[1];
    g_tty.active = true;

    struct sigaction action{};
    sigemptyset(&action.sa_mask);
    action.sa_handler = on_fatal_signal;
    for (std::size_t i = 0; i < std::size(fatal_signals); ++i) {
      ::sigaction(fatal_signals[i], &action, &g_old_actions[i]);
    }
    action.sa_handler = on_winch;
    action.sa_flags = SA_RESTART;
    ::sigaction(SIGWINCH, &action, &g_old_winch);

    write_all(fd_, enter_sequence.data(), enter_sequence.size());
    update_size();
  }

  ~im_backend_ansi() override {
    restore_terminal();
    for (std::size_t i = 0; i < std::size(fatal_signals); ++i) {
      ::sigaction(fatal_signals[i], &g_old_actions[i], nullptr);
    }
    ::sigaction(SIGWINCH, &g_old_winch, nullptr);
    g_tty = {};
    ::close(resize_pipe_[0]);
    ::close(resize_pipe_[1]);
    ::close(fd_);
  }

  im_backend_ansi(im_backend_ansi const&) = delete;
  im_backend_ansi& operator=(im_backend_ansi const&) = delete;

  [[nodiscard]] auto size() const -> im_vec2 override {
    return screen_.size();
  }

  auto poll_events(im_input& input, std::chrono::milliseconds timeout) -> bool override {
    auto got = false;
    auto first = true;
    while (true) {
      // block only before the first event, then drain what is already there
      auto wait = first ? int(std::clamp<std::int64_t>(timeout.count(), -1, INT32_MAX)) : 0;
      if (parser_.pending()) {
        wait = esc_timeout_ms_; // the rest of an escape sequence is on its way (or it was the Esc key)
      }

      pollfd fds[2] = {{.fd = fd_, .events = POLLIN, .revents = 0}, {.fd = resize_pipe_[0], .events = POLLIN, .revents = 0}};
      auto const rc = ::poll(fds, 2, wait);
      if (rc < 0) {
        if (errno == EINTR) {
          continue;
        }
        throw system_error("poll");
      }
      if (rc == 0) {
        if (parser_.pending()) {
          got = parser_.flush(input) || got;
        }
        return got;
      }
      first = false;

      if (fds[1].revents & POLLIN) {
        char drain[64];
        while (::read(resize_pipe_[0], drain, sizeof(drain)) > 0) {
        }
        update_size();
        got = true;
      }
      if (fds[0].revents & POLLIN) {
        char bytes[4096];
        auto const n = ::read(fd_, bytes, sizeof(bytes));
        if (n < 0 && errno != EINTR && errno != EAGAIN) {
          throw system_error("read terminal");
        }
        if (n > 0) {
          got = parser_.feed(std::string_view(bytes, std::size_t(n)), input) || got;
        }
      } else if (fds[0].revents & (POLLHUP | POLLERR | POLLNVAL)) {
        throw std::runtime_error("terminal closed");
      }
    }
  }

  void clear(im_style const& style) override {
    screen_.clear(style);
  }

  void set_cell(int x, int y, std::uint32_t ch, im_style const& style) override {
    screen_.set_cell(x, y, ch, style);
  }

  void present() override {
    screen_.present(out_);
    if (!out_.empty()) {
      write_all(fd_, out_.data(), out_.size());
      out_.clear(); // keeps capacity
    }
  }

  void wake_up() override {
    // same self-pipe as SIGWINCH: poll() returns, the frame is rebuilt
    char const byte = 2;
    [[maybe_unused]] auto const n = ::write(resize_pipe_[1], &byte, 1);
  }

  void set_clipboard(std::string_view text) override {
    // sent with the next frame, like any other output
    out_ += osc52_sequence(text, std::getenv("TMUX") != nullptr);
  }

private:
  void update_size() {
    winsize ws{};
    if (::ioctl(fd_, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0 || ws.ws_row == 0) {
      ws.ws_col = 80, ws.ws_row = 24;
    }
    if (screen_.size() != im_vec2(ws.ws_col, ws.ws_row)) {
      screen_.resize(im_vec2(ws.ws_col, ws.ws_row)); // full redraw on next present
    }
  }

  int fd_ = -1;
  int resize_pipe_[2] = {-1, -1};
  int esc_timeout_ms_ = esc_timeout_from_env();
  ansi_input_parser parser_;
  ansi_screen screen_;
  std::string out_;
};

} // namespace

auto make_ansi_backend() -> std::unique_ptr<im_backend> {
  return std::make_unique<im_backend_ansi>();
}

} // namespace xxx
