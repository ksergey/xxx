// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx2.h"

#include <cassert>
#include <span>
#include <stdexcept>
#include <vector>

#include "xxx_draw.h"
#include "xxx_input.h"
#include "xxx_internal.h"
#include "xxx_layout.h"
#include "xxx_unicode.h"

namespace xxx::v2 {
namespace {

void handle_terminal_key_event(::tb_event const& event) {
  switch (event.key) {
  case TB_KEY_TAB:
    return add_key_event(im_key_id::tab);
  case TB_KEY_ENTER:
    return add_key_event(im_key_id::enter);
  case TB_KEY_ESC:
    return add_key_event(im_key_id::esc);
  case TB_KEY_CTRL_Q:
    return add_key_event(im_key_id::quit);
  case TB_KEY_CTRL_C:
    return add_key_event(im_key_id::quit);
  default:
    break;
  }
}

void handle_terminal_mouse_event(::tb_event const& event) {
  add_mouse_pos_event(event.x, event.y);

  if (event.key > 0) {
    switch (event.key) {
    case TB_KEY_MOUSE_LEFT:
      return add_mouse_button_event(im_mouse_button_id::left, event.x, event.y);
    case TB_KEY_MOUSE_RIGHT:
      return add_mouse_button_event(im_mouse_button_id::right, event.x, event.y);
    case TB_KEY_MOUSE_MIDDLE:
      return add_mouse_button_event(im_mouse_button_id::middle, event.x, event.y);
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

  auto const& layouts_stack = ctx->layouts_stack;
  assert(!layouts_stack.empty());

  return layouts_stack.front().size;
}

void init() {
  // TODO: return std::expected?

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

  clear_input_keys();
  clear_input_mouse();

  ::tb_event event;
  while (true) {
    auto const rc = ::tb_peek_event(&event, static_cast<int>(timeout.count()));
    if (rc == TB_OK) {
      switch (event.type) {
      case TB_EVENT_KEY: {
        if (event.ch > 0) {
          add_input_character(event.ch);
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
      };
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
  clear_layout();

  ::tb_clear();
}

void render() {
  ::tb_present();
}

void layout_row_begin(int min_height, std::size_t columns) {}

void layout_row_push(float width_or_ratio) {}

void layout_row_end() {}

void label(std::string_view text) {
  auto rect = reserve_space(1);
  if (rect.empty() || text.empty()) {
    return;
  }

  auto const glyphs = utf8_to_unicode(text);
  auto const glyphs_length = static_cast<int>(glyphs.size());

  if (auto const width = rect.get_width(); glyphs_length < width) {
    rect.max.x -= (width - glyphs_length);
  }

  // TODO
  auto const hovered = is_mouse_hovering_rect(rect);
  auto const clicked = is_mouse_pressed(im_mouse_button_id::left);
  auto const style = make_style(hovered ? (clicked ? 0x99ffee_c : 0xff9999_c) : 0xee6666_c);

  if (auto const width = rect.get_width(); glyphs_length > width) {
    draw_text(rect.min.x, rect.min.y, glyphs.subspan(0, width), style);
  } else {
    draw_text(rect.min.x, rect.min.y, glyphs, style);
  }

  commit_space(rect.get_height());
}

void show_debug() {
  auto const ctx = get_context();
  assert(ctx);

  auto const& mouse = ctx->input.mouse;
  label("mouse-pos: ({}, {})", mouse.pos.x, mouse.pos.y);
  label("mouse-prev: ({}, {})", mouse.prev.x, mouse.prev.y);

  auto const window_size = get_window_size();
  label("screen: ({}, {})", window_size.x, window_size.y);
}

} // namespace xxx::v2
