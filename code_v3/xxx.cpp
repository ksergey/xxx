// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <cassert>

#include "xxx-draw.h"
#include "xxx-input.h"
#include "xxx-internal.h"
#include "xxx-unicode.h"

namespace xxx {

auto get_default_color() noexcept -> im_color {
  return im_color(TB_DEFAULT);
}

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
  input_add_mouse_pos_event(im_vec2(event.x, event.y));

  if (event.key > 0) {
    switch (event.key) {
    case TB_KEY_MOUSE_LEFT:
      return input_add_mouse_button_event(im_mouse_button_id::left, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_RIGHT:
      return input_add_mouse_button_event(im_mouse_button_id::right, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_MIDDLE:
      return input_add_mouse_button_event(im_mouse_button_id::middle, im_vec2(event.x, event.y));
    default:
      break;
    }
  }
}

} // namespace

void init() {
  auto const ctx = get_context();
  assert(ctx);

  ctx->layouts = im_stack<im_layout>(64);

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

void update(std::chrono::milliseconds timeout) {
  auto const start = clock::now();
  auto const expiration = start + timeout;

  input_reset();

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

  ctx->layouts.resize(1);
  auto& layout = ctx->layouts.back();
  layout.type = im_layout_type::container;
  layout.bounds = im_rect(0, 0, ::tb_width() - 1, ::tb_height() - 1);
  layout.at = im_vec2(0, 0);

  ::tb_clear();
}

void render() {
  ::tb_present();
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

auto get_mouse_pos() -> im_vec2 {
  auto const ctx = get_context();
  assert(ctx);

  auto const& mouse = ctx->input.mouse;
  return mouse.pos;
}

auto get_window_size() -> im_vec2 {
  return im_vec2(::tb_width(), ::tb_height());
}

auto get_window_bounds() -> im_rect {
  auto const ctx = get_context();
  assert(ctx);
  return ctx->layouts.front().bounds;
}

void debug() {
  auto const glyphs = utf8_to_unicode("hello my friend");
  draw_text(im_vec2(1, 1), glyphs, im_style(make_color(.8, .5, 1.0)));

  draw_border(get_window_bounds(), im_style(make_color(0.8, 1.0, 0.5)));
  draw_border(im_rect(7, 7, 8, 8), im_style(make_color(0.3, 0.8, 1.0)));
  // // draw_border(im_rect(4, 4, 6, 6), im_style(make_color(0.3, 0.4, 0.8)));

  // auto rect = im_rect(5, 5, 5 + 2, 5 + 2);
  // draw_rect(rect, '.', im_style(make_color(.1, .5, .9), make_color(.8, 0.1, 0.2)));
  // draw_border(rect, im_style(make_color(0.3, 0.4, 0.8), make_color(0.3, 0.2, 0.3)));

  // draw_hline(rect.top_left(), 2, '-', im_style());
  // draw_hline(rect.bottom_left(), 2, '-', im_style());
}

} // namespace xxx
