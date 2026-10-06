// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// The application owns the event loop: one thread, one poll() over the terminal and its own
// descriptors (here a pipe fed by a child process, standing in for a socket). No threads, no wake_up().

#include <chrono>
#include <csignal>
#include <cstdio>
#include <deque>
#include <format>
#include <string>

#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

#include <terminal.h>
#include <xxx.h>

namespace {

// child: one line every 500 ms, like a remote peer
[[noreturn]] void producer(int out) {
  for (int i = 1;; ++i) {
    auto const line = std::format("message {} from pid {}\n", i, ::getpid());
    if (::write(out, line.data(), line.size()) < 0) {
      ::_exit(0);
    }
    ::usleep(500'000);
  }
}

} // namespace

int main() {
  int channel[2];
  if (::pipe(channel) != 0) {
    return 1;
  }
  auto const child = ::fork();
  if (child == 0) {
    ::close(channel[0]);
    producer(channel[1]);
  }
  ::close(channel[1]);

  {
    auto term = xxx::terminal();
    xxx::init(term);

    auto messages = std::deque<std::string>();
    auto pending = std::string();
    auto presses = 0;
    auto quit = false;

    while (!quit) {
      // one wait for everything; the timeout only matters while a lone Esc is being resolved
      pollfd fds[] = {
          {.fd = term.input_fd(), .events = POLLIN, .revents = 0},
          {.fd = term.notify_fd(), .events = POLLIN, .revents = 0},
          {.fd = channel[0], .events = POLLIN, .revents = 0},
      };
      ::poll(fds, 3, term.timeout_ms());

      if (fds[2].revents & POLLIN) {
        char buffer[512];
        if (auto const n = ::read(channel[0], buffer, sizeof(buffer)); n > 0) {
          pending.append(buffer, std::size_t(n));
          for (auto eol = pending.find('\n'); eol != std::string::npos; eol = pending.find('\n')) {
            messages.push_front(pending.substr(0, eol));
            pending.erase(0, eol + 1);
          }
          if (messages.size() > 200) {
            messages.resize(200);
          }
        }
      }
      term.read_available();
      term.check_timeout();

      xxx::process_input_events(); // never waits here: takes what the terminal queued
      if (xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
        quit = true;
      }

      xxx::new_frame();
      xxx::view_begin("poll loop");
      xxx::label("terminal, notify and a pipe are waited on by one poll() in one thread");
      if (xxx::button("press me")) {
        ++presses;
      }
      xxx::same_line();
      xxx::label(std::format("pressed {} times", presses));
      xxx::view_end();
      xxx::view_begin(std::format("{} messages from the pipe##log", messages.size()), xxx::im_view_flags_default,
          xxx::im_key_id(), xxx::fill(1));
      for (auto const& m : messages) {
        xxx::label(m);
      }
      xxx::view_end();
      xxx::key_hint("c-q", "quit");
      xxx::key_hints("c-q quit");
      xxx::render();
    }
    xxx::shutdown();
  } // terminal restored here

  ::kill(child, SIGTERM);
  ::waitpid(child, nullptr, 0);
  return 0;
}
