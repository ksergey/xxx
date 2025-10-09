// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <cassert>

#include "im_context.h"

namespace xxx {

namespace {

void handle_terminal_key_event(::tb_event const& event) {
  switch (event.key) {
  case TB_KEY_TAB:
    return g_ctx->input.add_key_event(im_key_id::tab);
  case TB_KEY_ENTER:
    return g_ctx->input.add_key_event(im_key_id::enter);
  case TB_KEY_ESC:
    return g_ctx->input.add_key_event(im_key_id::esc);
  case TB_KEY_CTRL_Q:
    return g_ctx->input.add_key_event(im_key_id::quit);
  case TB_KEY_CTRL_C:
    return g_ctx->input.add_key_event(im_key_id::quit);
  case TB_KEY_SPACE:
    return g_ctx->input.add_key_event(im_key_id::space);
  default:
    break;
  }
}

void handle_terminal_mouse_event(::tb_event const& event) {
  g_ctx->input.add_mouse_pos_event(im_vec2(event.x, event.y));

  if (event.key > 0) {
    switch (event.key) {
    case TB_KEY_MOUSE_LEFT:
      return g_ctx->input.add_mouse_button_event(im_mouse_button_id::left, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_RIGHT:
      return g_ctx->input.add_mouse_button_event(im_mouse_button_id::right, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_MIDDLE:
      return g_ctx->input.add_mouse_button_event(im_mouse_button_id::middle, im_vec2(event.x, event.y));
    default:
      break;
    }
  }
}

} // namespace

void init() {
  if (g_ctx) {
    delete g_ctx;
  }
  g_ctx = new im_context;

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

  delete g_ctx;
  g_ctx = nullptr;
}

void process_input_events() {
  g_ctx->input.reset();

  ::tb_event event;

  auto do_peek_events = true;
  while (do_peek_events) {
    auto const rc = ::tb_peek_event(&event, 0);
    if (rc == TB_OK) {
      switch (event.type) {
      case TB_EVENT_KEY: {
        if (event.ch > 0) {
          g_ctx->input.add_character(event.ch);
          if (event.ch == ' ') {
            g_ctx->input.add_key_event(im_key_id::space);
          }
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
      do_peek_events = false;
    } else if (rc == TB_ERR_POLL) {
      // handle poll error
      if (::tb_last_errno() != EINTR) {
        throw std::runtime_error(::tb_strerror(rc));
      }
    }
  }

  if (is_key_pressed(im_key_id::tab)) {
    auto& view = g_ctx->view;
    if (view.next_id != im_id()) {
      view.active_id = view.next_id;
    } else if (view.first_id != im_id()) {
      view.active_id = view.first_id;
    }
  }
}

void new_frame() {
  assert(g_ctx);

  // TODO: frame delta

  auto const screen_rect = im_rect(0, 0, ::tb_width() - 1, ::tb_height() - 1);

  g_ctx->hash_id.reset();
  g_ctx->theme.reset();
  g_ctx->layout.reset(screen_rect);

  g_ctx->view.current_name = "N/A";
  g_ctx->view.current_id = im_id();
  g_ctx->view.first_id = im_id();
  g_ctx->view.next_id = im_id();
  g_ctx->view.active = false;

  g_ctx->renderer.set_clear_color(g_ctx->theme.get_style(im_color_id::text, im_color_id::background));
  g_ctx->renderer.start_new_frame(screen_rect);
}

void render() {
  assert(g_ctx);

  g_ctx->renderer.render();
}

void debug() {
  g_ctx->renderer.cmd_draw_rect(im_rect(2, 2, 6, 6), im_style(0x3366ff_c));
  g_ctx->renderer.cmd_draw_rect(im_rect(8, 8, 9, 9), im_style(0x33ff66_c));
  auto text = utf8_to_unicode("hello world 1234");
  auto rect = g_ctx->renderer.clip_rect().crop(10);
  // g_ctx->renderer.push_clip_rect(im_rect(39, 24, 100, 100));
  g_ctx->renderer.cmd_draw_text_in_rect(
      rect, text, im_style(0xffee33_c, 0x332211), im_halign::center, im_valign::center);
}

auto get_screen_rect() -> im_rect {
  return im_rect(0, 0, ::tb_width() - 1, ::tb_height() - 1);
}

auto is_key_pressed(im_key_id id) -> bool {
  return g_ctx->input.is_key_pressed(id);
}

void set_default_color(im_color_id id, im_color color) {
  g_ctx->theme.set_default_color(id, color);
}

void push_color(im_color_id id, im_color color) {
  g_ctx->theme.push_color(id, color);
}

void pop_color(std::size_t cnt) {
  g_ctx->theme.pop_color(cnt);
}

void layout_row_begin(std::size_t columns) {
  g_ctx->layout.row_begin(columns);
}

void layout_row_push(float ratio_or_width) {
  g_ctx->layout.row_push(ratio_or_width);
}

void layout_row_end() {
  g_ctx->layout.row_end();
}

void view_begin(std::string_view name) {
  auto& view = g_ctx->view;
  if (view.current_id != im_id()) {
    assert(false && "view_begin(...) inside another view");
    return;
  }

  auto const [str, key] = g_ctx->hash_id.split_str_key(name);

  view.current_name = str;
  view.current_id = g_ctx->hash_id.push_id(key);
  if (view.active_id == im_id()) {
    view.active_id = view.current_id;
  }
  if (view.current_id != view.active_id && view.next_id == im_id()) {
    view.next_id = view.current_id;
  }
  if (view.first_id == im_id()) {
    view.first_id = view.current_id;
  }
  view.active = (view.active_id == view.current_id);
  if (view.active) {
    view.next_id = im_id();
  }
}

void view_end() {
  auto& view = g_ctx->view;

  g_ctx->hash_id.pop_id();

  view.current_name = "N/A";
  view.current_id = im_id();
  view.active = false;
}

auto view_get_name() -> std::string_view {
  return g_ctx->view.current_name;
}

auto view_is_active() -> bool {
  return g_ctx->view.active;
}

void label(std::string_view text) {
  auto const widget_rect = g_ctx->layout.add_widget_item(1);
  if (!g_ctx->renderer.is_visible(widget_rect)) {
    return;
  }
  auto const style = g_ctx->theme.get_style(im_color_id::text, im_color_id::background);
  g_ctx->renderer.cmd_draw_text_in_rect(widget_rect, utf8_to_unicode(text), style, im_halign::left, im_valign::top);
}

} // namespace xxx
