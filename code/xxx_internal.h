// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <array>
#include <utility>
#include <vector>

#include <termbox2.h>

#include "xxx2.h"

namespace xxx::v2 {

static_assert(std::is_same_v<uintattr_t, std::uint64_t>, "termbox2 invalid configuration");

// ------------------------------------
// common
// ------------------------------------

// clock type
using clock = std::chrono::steady_clock;

// ------------------------------------
// layout
// ------------------------------------
struct im_row_layout {
  int height;
  int min_height;
  int filled_width;
  std::size_t columns;
  std::size_t column;
};

enum class im_layout_type { container, row, column };

struct im_layout {
  im_vec2 pos;
  im_vec2 size;
  im_vec2 filled;
  std::size_t columns;
  std::size_t column;

  // new version
  im_layout_type type;
  im_rect bounds;
  im_row_layout row;
};

// ------------------------------------
// input
// ------------------------------------
struct im_key_state {
  std::size_t clicked = 0;
};

struct im_keyboard {
  static constexpr std::size_t max_keys = std::to_underlying(im_key_id::last);
  static constexpr std::size_t max_text_input = 16;

  std::array<im_key_state, max_keys> keys;
  std::array<uint32_t, max_text_input> text;
  std::size_t text_length = 0;
};

struct im_mouse_button_state {
  std::size_t clicked;
  im_vec2 clicked_pos;
};

struct im_mouse {
  static constexpr std::size_t max_buttons = std::to_underlying(im_mouse_button_id::last);

  std::array<im_mouse_button_state, max_buttons> buttons;
  im_vec2 pos = im_vec2{-1, -1};
  im_vec2 prev = im_vec2{-1, -1};
  im_vec2 delta;
};

struct im_input {
  im_keyboard keyboard;
  im_mouse mouse;
};

// ------------------------------------
// context
// ------------------------------------
struct im_context {
  im_input input;
  std::vector<im_layout> layouts_stack;
};

[[nodiscard]] inline auto get_context() -> im_context* {
  static im_context instance;
  return &instance;
}

} // namespace xxx::v2
