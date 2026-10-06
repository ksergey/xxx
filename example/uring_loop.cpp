// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// The terminal driven by io_uring (completion model): the application reads input itself and hands the
// bytes to the terminal, catches SIGWINCH with signalfd, writes frames asynchronously, and resolves a
// lone Esc with an io_uring timeout. One thread, one io_uring_wait_cqe().

#include <chrono>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <cstdio>
#include <format>
#include <string>
#include <stdexcept>
#include <string_view>

#include <liburing.h>
#include <poll.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include <terminal.h>
#include <xxx.h>

namespace {

// what a completion is about
enum tag : std::uint64_t { input = 1, resize, notify, esc_timer, clock_tick, output };

struct ring {
  io_uring r{};

  ring() {
    if (::io_uring_queue_init(16, &r, 0) != 0) {
      throw std::runtime_error("io_uring_queue_init");
    }
  }
  ~ring() {
    ::io_uring_queue_exit(&r);
  }
  auto sqe(tag t) -> io_uring_sqe* {
    auto* e = ::io_uring_get_sqe(&r);
    if (!e) {
      ::io_uring_submit(&r);
      e = ::io_uring_get_sqe(&r);
    }
    ::io_uring_sqe_set_data64(e, t);
    return e;
  }
};

} // namespace

int main() {
  // SIGWINCH through signalfd, so it is just another completion
  sigset_t winch;
  sigemptyset(&winch);
  sigaddset(&winch, SIGWINCH);
  ::sigprocmask(SIG_BLOCK, &winch, nullptr);
  auto const signals = ::signalfd(-1, &winch, SFD_CLOEXEC);

  auto uring = ring();
  // io_uring offset -1: "current position", the only one that makes sense for a tty
  constexpr auto stream = std::uint64_t(-1);
  auto tty = -1; // known once the terminal exists; it writes its setup itself, frames come later
  {
    auto term = xxx::terminal({
        .tty_fd = -1,
        .handle_resize_signal = false,
        .write = [&](std::string_view bytes) {
          // bytes stay valid until write_done()
          ::io_uring_prep_write(uring.sqe(output), tty, bytes.data(), unsigned(bytes.size()), stream);
        },
    });
    tty = term.input_fd();

    xxx::init(term);

    char input_buffer[4096];
    signalfd_siginfo siginfo{};
    auto const read_input = [&] { ::io_uring_prep_read(uring.sqe(input), tty, input_buffer, sizeof(input_buffer), stream); };
    auto const read_signal = [&] { ::io_uring_prep_read(uring.sqe(resize), signals, &siginfo, sizeof(siginfo), stream); };
    // notify_fd: the library asks for the next frame through it (popups, view switches)
    auto const poll_notify = [&] { ::io_uring_prep_poll_add(uring.sqe(notify), term.notify_fd(), POLLIN); };
    __kernel_timespec tick = {.tv_sec = 1, .tv_nsec = 0};
    auto const arm_clock = [&] { ::io_uring_prep_timeout(uring.sqe(clock_tick), &tick, 0, 0); };
    __kernel_timespec esc{};
    auto esc_armed = false;

    read_input();
    read_signal();
    poll_notify();
    arm_clock();

    auto events = 0L; // completions that may change what is on screen
    auto presses = 0;
    auto text = std::string();
    auto const started = std::chrono::steady_clock::now();
    auto quit = false;
    auto need_frame = true; // the first frame right away, then only when something happened

    while (!quit) {
      if (need_frame) {
        need_frame = false;
        xxx::process_input_events(); // takes the queued events, never waits
        if (xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
          break;
        }
        auto const uptime =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - started);
        xxx::new_frame();
        xxx::view_begin("io_uring loop");
        xxx::label(std::format("input, signalfd, notify_fd and timers are io_uring completions: {}", events));
        xxx::label(std::format("up {}", uptime));
        xxx::text_input("type here", text);
        if (xxx::button("press me")) {
          ++presses;
        }
        xxx::same_line();
        xxx::label(std::format("pressed {} times", presses));
        xxx::view_end();
        xxx::key_hint("c-q", "quit");
        xxx::key_hints("c-q quit");
        xxx::render(); // hands the frame to options::write, or holds it while a write is in flight
      }

      // a lone Esc waits for the rest of a key sequence: an io_uring timeout resolves it
      if (auto const ms = term.timeout_ms(); ms >= 0 && !esc_armed) {
        esc = {.tv_sec = 0, .tv_nsec = (long long)ms * 1'000'000};
        ::io_uring_prep_timeout(uring.sqe(esc_timer), &esc, 0, 0);
        esc_armed = true;
      }
      ::io_uring_submit(&uring.r);

      io_uring_cqe* cqe = nullptr;
      if (::io_uring_wait_cqe(&uring.r, &cqe) != 0) {
        continue;
      }
      // take everything completed so far, then build at most one frame
      unsigned head = 0, seen = 0;
      io_uring_for_each_cqe(&uring.r, head, cqe) {
        ++seen;
        auto const res = cqe->res;
        auto const what = ::io_uring_cqe_get_data64(cqe);
        if (what != output) {
          ++events;
          need_frame = true;
        }
        switch (what) {
        case input:
          if (res > 0) {
            term.feed(std::string_view(input_buffer, std::size_t(res)));
          }
          read_input();
          break;
        case resize:
          term.notify_resize();
          read_signal();
          break;
        case notify:
          term.read_notifications();
          poll_notify();
          break;
        case esc_timer:
          esc_armed = false;
          term.check_timeout();
          break;
        case clock_tick:
          arm_clock();
          break;
        case output:
          // a finished write needs no new frame: a held back one is handed by write_done() itself
          if (res >= 0) {
            term.write_done(std::size_t(res));
          } else if (res == -EINTR || res == -EAGAIN) {
            term.write_done(0); // hand the same bytes again
          } else {
            std::fprintf(stderr, "write: %s\n", std::strerror(-res));
            quit = true;
          }
          break;
        default:
          break;
        }
      }
      ::io_uring_cq_advance(&uring.r, seen);
    }
    xxx::shutdown();
  } // terminal restored here (all writes are done: each frame waits for write_done)
  ::close(signals);
  return 0;
}
