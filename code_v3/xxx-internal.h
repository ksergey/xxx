// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <utility>
#include <variant>

#include <termbox2.h>

#include "hash.h"
#include "xxx-renderer.h"
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

struct im_layout_row {
  int columns; // columns count
  int index;   // current column index
};

struct im_layout_none {};

enum class im_layout_type { container, column, row };

struct im_layout {
  im_layout_type type; // layout type
  im_rect bounds;      // layout bounds (global)

  union {
    im_layout_row row;
    im_layout_none none;
  };
};

struct im_color_state {
  im_color_id id;
  im_color color;
};

struct im_context {
  static constexpr auto max_color_states = std::to_underlying(im_color_id::last);

  im_input input;
  im_vec2 cursor;
  im_stack<im_layout> layout_stack;
  im_stack<im_vec2> cursor_stack;
  im_stack<im_color_state> color_state_stack;
  im_stack<std::uint32_t> hash_stack;

  std::array<im_color, max_color_states> colors;

  std::uint32_t next_focused_widget_id;
  std::uint32_t prev_focused_widget_id;
  std::uint32_t focused_widget_id;

  struct {
    std::uint32_t widget_id;
    im_rect bounds;
    bool visible;
    bool focused;
    bool hovered;
    bool pressed;
  } widget;

  im_renderer renderer;
};

[[nodiscard]] inline auto get_context() -> im_context* {
  static im_context instance;
  return &instance;
}

[[nodiscard]] inline auto get_id(std::string_view str) noexcept -> std::uint32_t {
  return hash(str, get_context()->hash_stack.back());
}

} // namespace xxx
