// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <utility>

#include <termbox2.h>

#include "xxx2.h"
#include "xxx_fixed_vector.h"

namespace xxx::v2 {

// ----------------------------------------------------------------------------
// common
// ----------------------------------------------------------------------------

static_assert(std::is_same_v<uintattr_t, std::uint64_t>, "termbox2 invalid configuration");

// clock type
using clock = std::chrono::steady_clock;

// ----------------------------------------------------------------------------
// input
// ----------------------------------------------------------------------------
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
  // TODO: down? see TB_KEY_MOUSE_RELEASE
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

// ----------------------------------------------------------------------------
// layout
// ----------------------------------------------------------------------------
struct im_row_layout {
  int filled_width; // row filled width
  int index;        // next column index
  int columns;      // columns count
};

enum class im_layout_type { container, row, column };

struct im_layout {
  im_layout_type type;
  im_rect bounds;
  int min_height;
  int filled_height;
  im_row_layout row;
};

// ----------------------------------------------------------------------------
// context
// ----------------------------------------------------------------------------
struct im_context {
  im_input input;
  im_fixed_vector<im_layout> layouts_stack;
};

[[nodiscard]] inline auto get_context() -> im_context* {
  static im_context instance;
  return &instance;
}

} // namespace xxx::v2
