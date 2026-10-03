// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "im_backend.h"

namespace xxx {

/// In-memory backend for tests: renders into cell buffer, input is injected
/// programmatically, time advances only on request.
class im_backend_headless final : public im_backend {
public:
  explicit im_backend_headless(im_vec2 size);

  // im_backend
  [[nodiscard]] auto size() const -> im_vec2 override;
  [[nodiscard]] auto now() const -> im_clock::time_point override;
  void poll_events(im_input& input) override;
  void clear(im_style const& style) override;
  void set_cell(int x, int y, std::uint32_t ch, im_style const& style) override;
  void present() override;

  /// Input queued here is delivered on next process_input_events()
  void push_key(im_key_id key);
  /// Typed character (space also produces im_key_id::space, like a terminal does)
  void push_char(std::uint32_t ch);
  /// Typed utf8 text, one event per codepoint
  void push_text(std::string_view text);
  void push_mouse_pos(im_vec2 pos);
  void push_mouse_button(im_mouse_button_id button, im_vec2 pos);
  /// Wheel steps: > 0 down, < 0 up
  void push_mouse_wheel(int delta, im_vec2 pos);

  /// Advance clock returned by now()
  void advance_time(std::chrono::milliseconds delta) noexcept;

  /// Change screen size (takes effect on next frame)
  void resize(im_vec2 size);

  /// Presented cell. Cell covered by wide char keeps whatever was set there and is not shown.
  [[nodiscard]] auto cell(int x, int y) const -> im_cell const&;

  /// Presented line as utf8 (as terminal shows it), trailing spaces trimmed
  [[nodiscard]] auto line(int y) const -> std::string;

  /// All presented lines joined with '\n', trailing empty lines trimmed
  [[nodiscard]] auto screen() const -> std::string;

  /// Number of present() calls
  [[nodiscard]] auto frames() const noexcept -> int {
    return frames_;
  }

private:
  struct pending_event {
    enum class type { key, character, mouse_pos, mouse_button, mouse_wheel } type;
    im_key_id key = im_key_id();
    std::uint32_t ch = 0;
    im_mouse_button_id button = im_mouse_button_id::left;
    im_vec2 pos = im_vec2();
    int wheel = 0;
  };

  im_vec2 size_;
  std::vector<im_cell> back_;
  std::vector<im_cell> front_;
  std::vector<pending_event> events_;
  im_clock::time_point now_ = im_clock::time_point();
  int frames_ = 0;
};

} // namespace xxx
