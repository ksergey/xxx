// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
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
  im_layout_type type; // layout type
  im_rect bounds;      // layout bounds
  int min_height;      // min layout height (used inside formatting rows)
  int filled_height;   // column/container filled height
  im_row_layout row;   // layout row data
};

// ----------------------------------------------------------------------------
// context
// ----------------------------------------------------------------------------
struct im_context {
  im_input input;
  im_fixed_vector<im_layout> layout_stack;
  im_fixed_vector<im_rect> clip_rect_stack;
  im_fixed_vector<im_vec2> translate_stack;
};

[[nodiscard]] inline auto get_context() -> im_context* {
  static im_context instance;
  return &instance;
}

// ----------------------------------------------------------------------------
// unicode fn
// ----------------------------------------------------------------------------

[[nodiscard]] auto utf8_to_unicode(std::string_view input) -> std::span<std::uint32_t const>;

// ----------------------------------------------------------------------------
// input fn
// ----------------------------------------------------------------------------

void input_add_key_event(im_key_id id);
void input_add_mouse_pos_event(int x, int y);
void input_add_mouse_button_event(im_mouse_button_id id, int x, int y);
void input_add_character(std::uint32_t ch);
void input_add_characters_utf8(char const* str);
void input_clear_keys();
void input_clear_mouse();

// ----------------------------------------------------------------------------
// layout fn
// ----------------------------------------------------------------------------

auto layout_space_prepare(int height = 0) -> im_rect;
void layout_space_commit(int height);
void layout_set_min_height(int height);
void layout_row_begin(int height, std::size_t columns);
void layout_row_push(float ratio_or_width);
void layout_row_end();

// ----------------------------------------------------------------------------
// clip rect fn
// ----------------------------------------------------------------------------

[[nodiscard]] inline auto clip_rect_get() -> im_rect const& {
  auto const ctx = get_context();
  assert(ctx);
  auto& clip_rect_stack = ctx->clip_rect_stack;
  assert(!clip_rect_stack.empty());
  return clip_rect_stack.back();
}

inline void clip_rect_push(im_rect const& rect) {
  auto const ctx = get_context();
  assert(ctx);
  auto& clip_rect_stack = ctx->clip_rect_stack;
  clip_rect_stack.emplace_back(rect.intersection(clip_rect_stack.back()));
}

inline void clip_rect_pop() {
  auto const ctx = get_context();
  assert(ctx);
  auto& clip_rect_stack = ctx->clip_rect_stack;
  assert(clip_rect_stack.size() > 1);
  clip_rect_stack.pop_back();
}

// ----------------------------------------------------------------------------
// translate fn
// ----------------------------------------------------------------------------

[[nodiscard]] inline auto translate_get() -> im_vec2 const& {
  auto const ctx = get_context();
  assert(ctx);
  auto& translate_stack = ctx->translate_stack;
  assert(!translate_stack.empty());
  return translate_stack.back();
}

inline void translate_push(im_vec2 const& point) {
  auto const ctx = get_context();
  assert(ctx);
  auto& translate_stack = ctx->translate_stack;
  translate_stack.emplace_back(translate_stack.back() + point);
}

inline void translate_pop() {
  auto const ctx = get_context();
  assert(ctx);
  auto& translate_stack = ctx->translate_stack;
  assert(translate_stack.size() > 1);
  translate_stack.pop_back();
}

// ----------------------------------------------------------------------------
// draw
// ----------------------------------------------------------------------------

void draw_fill_rect(im_rect const& rect, std::uint32_t ch, im_style const& style) noexcept;
void draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) noexcept;

inline void draw_point(im_vec2 const& pos, std::uint32_t ch, im_style const& style) noexcept {
  draw_fill_rect(im_rect(pos.x, pos.y, pos.x + 1, pos.y + 1), ch, style);
}

inline void draw_hline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {
  draw_fill_rect(im_rect(pos.x, pos.y, pos.x + length, pos.y + 1), ch, style);
}

inline void draw_vline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {
  draw_fill_rect(im_rect(pos.x, pos.y, pos.x + 1, pos.y + length), ch, style);
}

} // namespace xxx::v2
