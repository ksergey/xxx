// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

#include "xxx.h"

namespace xxx {

/// The terminal as an event source for any event loop (poll, epoll, kqueue, asio, libuv, ...).
/// Owns the terminal: raw mode, alternate screen, mouse, bracketed paste; restores it on destruction
/// and on fatal signals. Never blocks: the application waits on the descriptors itself.
///
///   xxx::terminal term;
///   xxx::init(term);                     // UI takes events from term, never waits
///   while (true) {
///     pollfd fds[] = {{term.input_fd(), POLLIN, 0}, {term.notify_fd(), POLLIN, 0}};
///     poll(fds, 2, term.timeout_ms());   // plus the application's own descriptors
///     term.read_available();
///     term.check_timeout();
///     xxx::process_input_events();       // or read term.next_event() directly
///     ... build ui, xxx::render()
///   }
///
/// Completion based loops (io_uring, asio async_read): the application reads input_fd() itself and passes
/// the bytes to feed(); output goes through options::write asynchronously, see write_done().
///
/// One terminal at a time. Call from one thread; wake_up() from any.
class terminal {
public:
  struct options {
    /// Terminal descriptor to use (not closed by terminal); -1: open /dev/tty
    int tty_fd = -1;
    /// Install a SIGWINCH handler. false: the application handles it (e.g. signalfd) and calls notify_resize()
    bool handle_resize_signal = true;
    /// Asynchronous output. Set: each frame's bytes are handed here and must stay unchanged until
    /// write_done(). While a write is in flight frames are not sent, the latest one goes after it.
    /// Not set: frames are written synchronously.
    /// A finished write needs no new frame (write_done() hands a held one itself): building a frame on
    /// every write completion turns into a busy loop as soon as frames differ.
    std::function<void(std::string_view bytes)> write = {};
  };

  terminal();
  explicit terminal(options opts);
  ~terminal();

  terminal(terminal const&) = delete;
  terminal& operator=(terminal const&) = delete;

  /// Readable when the terminal sent input
  [[nodiscard]] auto input_fd() const noexcept -> int;

  /// Readable when the window was resized or wake_up() was called
  [[nodiscard]] auto notify_fd() const noexcept -> int;

  /// Milliseconds until check_timeout() must be called (a lone Esc waits for the rest of a key
  /// sequence, see ESCDELAY), -1 when there is no deadline. Ready to pass to poll().
  [[nodiscard]] auto timeout_ms() const noexcept -> int;

  /// Read what is ready on both descriptors without blocking, queue the events
  /// @return true if events were queued or a notification (resize, wake_up) came
  auto read_available() -> bool;

  /// Completion model: parse bytes the application read from input_fd() itself (instead of read_available)
  void feed(std::string_view bytes);

  /// Take only the notifications (resize, wake_up) from notify_fd(), leave input_fd() alone
  /// @return true if a notification came
  auto read_notifications() -> bool;

  /// The window may have changed size (when the application handles SIGWINCH itself)
  void notify_resize();

  /// options::write finished: \c written bytes of the handed ones went out. Fewer than handed: the rest is
  /// handed again. All of them: the latest frame, if one was held back, is handed next.
  void write_done(std::size_t written);

  /// Resolve a lone Esc whose deadline passed
  /// @return true if events were queued
  auto check_timeout() -> bool;

  /// Take the next queued event
  [[nodiscard]] auto next_event(im_event& event) -> bool;

  /// Screen size in cells
  [[nodiscard]] auto size() const noexcept -> im_vec2;

  /// Thread-safe: make notify_fd() readable (e.g. new data for the ui from another thread)
  void wake_up() noexcept;

  struct impl;

private:
  friend struct terminal_access;
  std::unique_ptr<impl> impl_;
};

} // namespace xxx
