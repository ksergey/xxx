// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "im_input.h"

namespace xxx {

/// Terminal input parser for xterm compatible terminals (no terminfo).
/// Pure: bytes in, ordered events (im_event) out. Understands
///   UTF-8 text, control keys (ctrl-a..z, tab, enter, backspace),
///   CSI / SS3 keys (arrows, home, end, page up/down, delete, shift-tab, F1-F12) with modifiers,
///   Alt + character / control key (ESC prefix),
///   SGR mouse (1006): press, release, drag, wheel,
///   bracketed paste (2004): pasted text becomes characters, never keys (a newline doesn't press enter).
/// A sequence split between feed() calls is kept until the rest arrives.
class ansi_input_parser {
public:
  /// Parse \c bytes, append events to \c out
  /// @return true if any event was produced
  auto feed(std::string_view bytes, std::vector<im_event>& out) -> bool;

  /// feed() straight into a frame input state
  auto feed(std::string_view bytes, im_input& input) -> bool {
    auto events = std::vector<im_event>();
    auto const produced = feed(bytes, events);
    for (auto const& e : events) {
      apply_event(e, input);
    }
    return produced;
  }

  /// True when a lone ESC (or an unfinished sequence) waits for more bytes.
  /// The caller waits a little (esc_timeout) and then calls flush().
  [[nodiscard]] auto pending() const noexcept -> bool {
    return !buffer_.empty();
  }

  /// No more bytes came: a lone ESC is the Esc key, ESC + char is Alt+char (char is kept),
  /// other unfinished data is dropped.
  /// @return true if any event was produced
  auto flush(std::vector<im_event>& out) -> bool;

  auto flush(im_input& input) -> bool {
    auto events = std::vector<im_event>();
    auto const produced = flush(events);
    for (auto const& e : events) {
      apply_event(e, input);
    }
    return produced;
  }

private:
  enum class result { done, incomplete };

  // parse one item from the start of buffer_; sets consumed_ on done
  struct sink;
  auto parse_one(sink& input) -> result;
  auto parse_csi(sink& input) -> result;
  void handle_paste_bytes(std::string_view bytes, sink& input);

  std::string buffer_;
  std::size_t consumed_ = 0;
  bool produced_ = false;
  bool in_paste_ = false;
  bool alt_next_ = false; // ESC prefix: the next char or control key is pressed with Alt
};

} // namespace xxx
