// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <utility>
#include <variant>

#include <termbox2.h>

#include "xxx-stack.h"
#include "xxx.h"

namespace xxx {

static_assert(std::is_same_v<uintattr_t, std::uint64_t>, "termbox2 invalid configuration");

using clock = std::chrono::steady_clock;

struct im_key_state {
  std::size_t clicked = 0;
};

struct im_keyboard {
  static constexpr std::size_t max_keys = std::to_underlying(im_key_id::last);
  static constexpr std::size_t max_text_input = 16;

  std::array<im_key_state, max_keys> keys;
  std::array<std::uint32_t, max_text_input> text;
  std::size_t text_length = 0;
};

struct im_mouse_button_state {
  std::size_t clicked;
  im_vec2 clicked_pos;
  // TODO: down? see TB_KEY_MOUSE_RELEASE
};

struct im_mouse {
  static constexpr std::size_t max_buttons = std::to_underlying(im_mouse_button_id::last);

  std::array<im_mouse_button_state, max_buttons> buttons;
  im_vec2 pos = im_vec2(-1, -1);
  im_vec2 prev = im_vec2(-1, -1);
  im_vec2 delta;
};

struct im_input {
  im_keyboard keyboard;
  im_mouse mouse;
};

struct im_style {
  std::uint64_t fg = TB_DEFAULT;
  std::uint64_t bg = TB_DEFAULT;

  constexpr im_style() = default;
  constexpr im_style(im_color fg0, im_color bg0 = im_color(TB_DEFAULT)) noexcept
      : fg(static_cast<std::uint64_t>(fg0)), bg(static_cast<std::uint64_t>(bg0)) {}
};

enum class im_layout_type { container, column, row };

struct im_layout {
  im_layout_type type = im_layout_type::container;
  im_rect bounds;
  im_vec2 at;
};

struct im_context {
  im_input input;
  im_stack<im_layout> layouts;
};

[[nodiscard]] inline auto get_context() -> im_context* {
  static im_context instance;
  return &instance;
}

} // namespace xxx
