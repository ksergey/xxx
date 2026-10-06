// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// xxx::terminal on a pseudo terminal: the test writes into the master side what a terminal would send

#include <chrono>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <doctest/doctest.h>

#include "terminal.h"
#include "test_utils.h"

namespace xxx::testing {

namespace {

using namespace std::chrono_literals;

struct pseudo_terminal {
  int master = -1;
  int slave = -1;

  pseudo_terminal() {
    master = ::posix_openpt(O_RDWR | O_NOCTTY);
    REQUIRE(master >= 0);
    REQUIRE(::grantpt(master) == 0);
    REQUIRE(::unlockpt(master) == 0);
    slave = ::open(::ptsname(master), O_RDWR | O_NOCTTY);
    REQUIRE(slave >= 0);
    resize(40, 10);
  }
  ~pseudo_terminal() {
    ::close(slave);
    ::close(master);
  }
  void resize(int cols, int rows) const {
    auto ws = winsize{};
    ws.ws_col = std::uint16_t(cols);
    ws.ws_row = std::uint16_t(rows);
    ::ioctl(master, TIOCSWINSZ, &ws);
  }
  // the terminal sends input
  void type(std::string_view bytes) const {
    REQUIRE(::write(master, bytes.data(), bytes.size()) == ssize_t(bytes.size()));
  }
  // what the application wrote to the terminal so far
  auto output() const -> std::string {
    auto result = std::string();
    pollfd p = {.fd = master, .events = POLLIN, .revents = 0};
    while (::poll(&p, 1, 50) > 0 && (p.revents & POLLIN)) {
      char buffer[4096];
      auto const n = ::read(master, buffer, sizeof(buffer));
      if (n <= 0) {
        break;
      }
      result.append(buffer, std::size_t(n));
    }
    return result;
  }
};

auto wait_readable(int fd) -> bool {
  pollfd p = {.fd = fd, .events = POLLIN, .revents = 0};
  return ::poll(&p, 1, 500) > 0;
}

auto drain(terminal& term) -> std::vector<im_event> {
  auto result = std::vector<im_event>();
  for (im_event e; term.next_event(e);) {
    result.push_back(e);
  }
  return result;
}

} // namespace

TEST_SUITE("terminal: readiness model") {

  TEST_CASE("read_available turns input into events") {
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave});
    pty.output(); // enter sequence
    pty.type("a\x1b[A");
    REQUIRE(wait_readable(term.input_fd()));
    CHECK(term.read_available());
    auto const events = drain(term);
    REQUIRE(events.size() == 2);
    CHECK(events[0].ch == 'a');
    CHECK(events[1].key == im_key_id::arrow_up);
  }

  TEST_CASE("nothing ready: nothing queued, no blocking") {
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave});
    CHECK_FALSE(term.read_available());
    CHECK(drain(term).empty());
    CHECK(term.timeout_ms() == -1);
  }

  TEST_CASE("size comes from the terminal") {
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave});
    CHECK(term.size() == im_vec2(40, 10));
  }

  TEST_CASE("wake_up makes notify_fd readable") {
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave});
    term.wake_up();
    REQUIRE(wait_readable(term.notify_fd()));
    CHECK(term.read_notifications());
    CHECK(drain(term).empty()); // a wake up is not an input event
  }

  TEST_CASE("terminal settings are restored") {
    auto pty = pseudo_terminal();
    termios before{}, after{};
    ::tcgetattr(pty.slave, &before);
    {
      auto term = terminal({.tty_fd = pty.slave});
      termios raw{};
      ::tcgetattr(pty.slave, &raw);
      CHECK((raw.c_lflag & ICANON) == 0);
    }
    ::tcgetattr(pty.slave, &after);
    CHECK(before.c_lflag == after.c_lflag);
    CHECK(before.c_iflag == after.c_iflag);
    CHECK(pty.output().find("\x1b[?1049l") != std::string::npos); // left the alternate screen
  }
}

TEST_SUITE("terminal: completion model") {

  TEST_CASE("feed parses bytes read elsewhere") {
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave});
    term.feed("x\x1b[<0;5;2M");
    auto const events = drain(term);
    REQUIRE(events.size() == 3);
    CHECK(events[0].ch == 'x');
    CHECK(events[2].kind == im_event::type::mouse_press);
    CHECK(events[2].pos == im_vec2(4, 1));
  }

  TEST_CASE("lone esc: deadline, then check_timeout") {
    ::setenv("ESCDELAY", "20", 1);
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave});
    ::unsetenv("ESCDELAY");
    term.feed("\x1b");
    CHECK(drain(term).empty());
    auto const timeout = term.timeout_ms();
    CHECK(timeout >= 0);
    CHECK(timeout <= 20);
    CHECK_FALSE(term.check_timeout()); // too early
    std::this_thread::sleep_for(30ms);
    CHECK(term.check_timeout());
    auto const events = drain(term);
    REQUIRE(events.size() == 1);
    CHECK(events[0].key == im_key_id::esc);
    CHECK(term.timeout_ms() == -1);
  }

  TEST_CASE("esc followed by the rest of a sequence is not Esc") {
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave});
    term.feed("\x1b");
    term.feed("[B");
    auto const events = drain(term);
    REQUIRE(events.size() == 1);
    CHECK(events[0].key == im_key_id::arrow_down);
  }

  TEST_CASE("resize handled by the application") {
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave, .handle_resize_signal = false});
    pty.resize(60, 20);
    term.notify_resize();
    auto const events = drain(term);
    REQUIRE(events.size() == 1);
    CHECK(events[0].kind == im_event::type::resize);
    CHECK(events[0].pos == im_vec2(60, 20));
    CHECK(term.size() == im_vec2(60, 20));
    term.notify_resize(); // same size: no event
    CHECK(drain(term).empty());
  }
}

TEST_SUITE("terminal: output") {

  TEST_CASE("synchronous: frames go straight to the terminal") {
    auto pty = pseudo_terminal();
    auto term = terminal({.tty_fd = pty.slave});
    pty.output();
    init(term);
    process_input_events();
    new_frame();
    label("hello");
    render();
    shutdown();
    CHECK(pty.output().find("hello") != std::string::npos);
  }

  TEST_CASE("asynchronous: one write in flight, latest frame after it, partial writes completed") {
    auto pty = pseudo_terminal();
    auto writes = std::vector<std::string>();
    auto term = terminal({.tty_fd = pty.slave, .write = [&](std::string_view bytes) { writes.emplace_back(bytes); }});
    init(term);
    auto const frame = [](std::string_view text) {
      process_input_events();
      new_frame();
      label(text);
      render();
    };

    frame("first");
    REQUIRE(writes.size() == 1);
    CHECK(writes[0].find("first") != std::string::npos);

    frame("second"); // in flight: held back
    frame("third");
    CHECK(writes.size() == 1);

    auto const half = writes[0].size() / 2;
    term.write_done(half); // partial: the rest is handed again
    REQUIRE(writes.size() == 2);
    CHECK(writes[1] == writes[0].substr(half));

    term.write_done(writes[1].size()); // done: the latest frame goes, the middle one is skipped
    REQUIRE(writes.size() == 3);
    CHECK(writes[2].find("third") != std::string::npos);
    CHECK(writes[2].find("second") == std::string::npos);

    term.write_done(writes[2].size());
    CHECK(writes.size() == 3); // nothing held
    shutdown();
  }

  TEST_CASE("asynchronous: clipboard queued during a write goes after it") {
    auto pty = pseudo_terminal();
    auto writes = std::vector<std::string>();
    auto term = terminal({.tty_fd = pty.slave, .write = [&](std::string_view bytes) { writes.emplace_back(bytes); }});
    init(term);
    process_input_events();
    new_frame();
    label("x");
    render();
    REQUIRE(writes.size() == 1);
    set_clipboard("copied");
    term.write_done(writes[0].size());
    REQUIRE(writes.size() == 2);
    CHECK(writes[1].find("\x1b]52;c;") != std::string::npos);
    shutdown();
  }
}

} // namespace xxx::testing
