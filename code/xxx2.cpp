// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx2.h"

#include <cassert>
#include <span>
#include <stdexcept>
#include <vector>

#include "xxx_internal.h"

namespace xxx::v2 {
namespace {

void handle_terminal_key_event(::tb_event const& event) {
  switch (event.key) {
  case TB_KEY_TAB:
    return input_add_key_event(im_key_id::tab);
  case TB_KEY_ENTER:
    return input_add_key_event(im_key_id::enter);
  case TB_KEY_ESC:
    return input_add_key_event(im_key_id::esc);
  case TB_KEY_CTRL_Q:
    return input_add_key_event(im_key_id::quit);
  case TB_KEY_CTRL_C:
    return input_add_key_event(im_key_id::quit);
  default:
    break;
  }
}

void handle_terminal_mouse_event(::tb_event const& event) {
  input_add_mouse_pos_event(event.x, event.y);

  if (event.key > 0) {
    switch (event.key) {
    case TB_KEY_MOUSE_LEFT:
      return input_add_mouse_button_event(im_mouse_button_id::left, event.x, event.y);
    case TB_KEY_MOUSE_RIGHT:
      return input_add_mouse_button_event(im_mouse_button_id::right, event.x, event.y);
    case TB_KEY_MOUSE_MIDDLE:
      return input_add_mouse_button_event(im_mouse_button_id::middle, event.x, event.y);
    default:
      break;
    }
  }
}

} // namespace

auto get_default_color() noexcept -> im_color {
  return im_color(TB_DEFAULT);
}

auto is_key_pressed(im_key_id id) -> bool {
  assert(id < im_key_id::last);

  auto const ctx = get_context();
  assert(ctx);

  auto& keyboard = ctx->input.keyboard;
  return keyboard.keys[static_cast<std::size_t>(id)].clicked > 0;
}

auto is_mouse_pressed(im_mouse_button_id id) -> bool {
  assert(id < im_mouse_button_id::last);

  auto const ctx = get_context();
  assert(ctx);

  auto const& mouse = ctx->input.mouse;
  return mouse.buttons[static_cast<std::size_t>(id)].clicked > 0;
}

auto is_mouse_hovering_rect(im_rect const& rect) -> bool {
  return rect.contains(get_mouse_pos());
}

auto get_mouse_pos() -> im_vec2 {
  auto const ctx = get_context();
  assert(ctx);

  auto const& mouse = ctx->input.mouse;
  return mouse.pos;
}

auto get_window_size() -> im_vec2 {
  auto const ctx = get_context();
  assert(ctx);

  auto const& layout_stack = ctx->layout_stack;
  assert(!layout_stack.empty());

  return layout_stack.front().bounds.get_size();
}

[[nodiscard]] auto layout_get_space_bounds() -> im_rect {
  return layout_space_prepare();
}

void init() {
  auto const ctx = get_context();
  assert(ctx);

  ctx->layout_stack = im_fixed_vector<im_layout>(64);
  ctx->clip_rect_stack = im_fixed_vector<im_rect>(64);
  ctx->translate_stack = im_fixed_vector<im_vec2>(64);

  // init termbox2 library
  if (auto const rc = ::tb_init(); rc != TB_OK) {
    throw std::runtime_error(::tb_strerror(rc));
  }
  ::tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
  ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
  ::tb_sendf("\x1b[?%d;%dh", 1003, 1006);
}

void shutdown() {
  ::tb_sendf("\x1b[?%d;%dl", 1003, 1006);
  ::tb_shutdown();
}

void poll_terminal_events(std::chrono::milliseconds timeout) {
  auto const start = clock::now();
  auto const expiration = start + timeout;

  input_clear_keys();
  input_clear_mouse();

  ::tb_event event;
  while (true) {
    auto const rc = ::tb_peek_event(&event, static_cast<int>(timeout.count()));
    if (rc == TB_OK) {
      switch (event.type) {
      case TB_EVENT_KEY: {
        if (event.ch > 0) {
          input_add_character(event.ch);
        } else if (event.key > 0) {
          handle_terminal_key_event(event);
        }
      } break;
      case TB_EVENT_MOUSE: {
        handle_terminal_mouse_event(event);
      } break;
      case TB_EVENT_RESIZE: {
      } break;
      default:
        break;
      }
    } else if (rc == TB_ERR_NO_EVENT) {
      // nothing to do
    } else if (rc == TB_ERR_POLL) {
      if (::tb_last_errno() != EINTR) {
        throw std::runtime_error(::tb_strerror(rc));
      }
    }

    auto const now = clock::now();
    if (now >= expiration) {
      break;
    }

    timeout = std::chrono::duration_cast<std::chrono::milliseconds>(expiration - now);
  }
}

void new_frame() {
  auto const ctx = get_context();
  assert(ctx);

  auto& layout_stack = ctx->layout_stack;
  layout_stack.resize(1);
  auto& layout = layout_stack.back();
  layout.type = im_layout_type::container;
  layout.bounds = im_rect(0, 0, ::tb_width(), ::tb_height());
  layout.min_height = 0;
  layout.filled_height = 0;
  layout.row.filled_width = 0;
  layout.row.index = 0;
  layout.row.columns = 0;

  auto& clip_rect_stack = ctx->clip_rect_stack;
  clip_rect_stack.resize(1);
  clip_rect_stack.back() = im_rect(0, 0, ::tb_width(), ::tb_height());

  auto& translate_stack = ctx->translate_stack;
  translate_stack.resize(1);
  translate_stack.back() = im_vec2(0, 0);

  ::tb_clear();
}

void render() {
  ::tb_present();
}

void debug() {
  auto const ctx = get_context();
  assert(ctx);

  auto const& mouse = ctx->input.mouse;
  auto const window_size = get_window_size();

  layout_row_begin(0, 3);
  layout_row_push(0.25f);
  layout_set_min_height(3);
  label("mouse-pos: ({}, {})", mouse.pos.x, mouse.pos.y);
  layout_row_end();

  layout_row_begin(0, 3);
  layout_row_push(0.25f);
  layout_row_push(0.50f);
  label("mouse-prev: ({}, {})", mouse.prev.x, mouse.prev.y);
  layout_row_push(0.25f);
  label("-");
  layout_row_end();

  layout_row_begin(0, 3);
  layout_row_push(0.75f);
  label("-");
  layout_row_push(0.25f);
  label("screen: ({}, {})", window_size.x, window_size.y);
  layout_row_end();
}

void debug_xxx() {
  {
    auto rect = layout_space_prepare();
    if (rect.empty_area()) {
      return;
    }

    auto rect1 = im_rect(0, 0, 10, 5);
    auto rect2 = im_rect(10, 0, 20, 5);

    auto const style1 = im_style(0xee99ff_c, 0x333333_c);
    auto const style2 = im_style(0xee99ff_c, 0x111133_c);

    auto clip_rect = rect;
    clip_rect.min.x += 1;
    clip_rect.min.y += 1;
    clip_rect.max.x = clip_rect.min.x + 12;
    clip_rect.max.y = clip_rect.min.y + 2;

    translate_push(rect.top_left());
    clip_rect_push(clip_rect);

    draw_fill_rect(rect1, '.', style1);
    draw_fill_rect(rect2, '.', style2);

    clip_rect_pop();
    translate_pop();

    layout_space_commit(7);
  }

  {
    auto rect = layout_space_prepare();
    if (rect.empty_area()) {
      return;
    }

    auto clip_rect = rect;
    clip_rect.min.x += 5;
    clip_rect.max.x = clip_rect.min.x + 3;

    auto const glyphs = utf8_to_unicode("aabbccddeeff");
    auto const style = im_style(0xcceeaa_c, 0x113344_c);

    clip_rect_push(clip_rect);
    draw_text(rect.get_pos(), glyphs, style);
    clip_rect_pop();

    layout_space_commit(1);
  }
}

void debug_rect() {
  auto const bounds = layout_space_prepare().adjusted_bottom(-1);
  auto const style = im_style(get_default_color(), 0x222222_c);

  draw_hline(bounds.top_left(), bounds.get_width(), '-', style);
  draw_hline(bounds.bottom_left(), bounds.get_width(), '-', style);
  draw_vline(
      bounds.adjusted_left(-bounds.get_width() / 2).adjusted_top(-1).top_left(), bounds.get_height() - 1, '|', style);

  layout_space_commit(bounds.get_height());
}

} // namespace xxx::v2
