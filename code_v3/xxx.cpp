// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <cassert>

#include "xxx-input.h"
#include "xxx-internal.h"
#include "xxx-layout.h"
#include "xxx-unicode.h"

namespace xxx {

namespace {

[[nodiscard]] constexpr auto get_content_and_id(
    std::string_view str) -> std::tuple<std::string_view, std::string_view> {
  auto const found = str.rfind("##");
  if (found != str.npos) {
    return std::make_tuple(str.substr(0, found), str.substr(found));
  } else {
    return std::make_tuple(str, str);
  }
}

} // namespace

auto get_default_color() noexcept -> im_color {
  return im_color(TB_DEFAULT);
}

[[nodiscard]] auto get_color(im_color_id id) noexcept -> im_color const& {
  assert(id < im_color_id::last);

  auto const index = static_cast<std::size_t>(id);

  auto const ctx = get_context();
  return ctx->colors[index];
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
  case TB_KEY_SPACE:
    return input_add_key_event(im_key_id::space);
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

void init(int flags) {
  auto const ctx = get_context();
  assert(ctx);

  constexpr auto stack_size = std::size_t(64);
  // TODO: dynamic?
  ctx->layout_stack = im_stack<im_layout>(stack_size);
  ctx->cursor_stack = im_stack<im_vec2>(stack_size);
  ctx->color_state_stack = im_stack<im_color_state>(stack_size);
  ctx->hash_stack = im_stack<std::uint32_t>(stack_size);

  ctx->hash_stack.push_back(0);

  ctx->colors.fill(get_default_color());

  ctx->next_focused_widget_id = 0;
  ctx->prev_focused_widget_id = 0;
  ctx->focused_widget_id = 0;

  // init termbox2 library
  if (auto const rc = ::tb_init(); rc != TB_OK) {
    throw std::runtime_error(::tb_strerror(rc));
  }

  auto const init_mouse = (flags & init_flags_mouse) == init_flags_mouse;

  ::tb_set_input_mode(TB_INPUT_ESC | (init_mouse ? TB_INPUT_MOUSE : 0));
  ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
  if (init_mouse) {
    ::tb_sendf("\x1b[?%d;%dh", 1003, 1006);
  }
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
          if (event.ch == ' ') {
            input_add_key_event(im_key_id::space);
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

  ctx->cursor = im_vec2(0, 0);

  // reset layout
  ctx->layout_stack.resize(1);
  auto& layout = ctx->layout_stack.back();
  layout.type = im_layout_type::container;
  layout.bounds = im_rect(0, 0, ::tb_width() - 1, ::tb_height() - 1);
  layout.none = {};

  ctx->cursor_stack.clear();

  // reset style
  ctx->color_state_stack.clear();

  // cleanup
  ctx->hash_stack.resize(1);

  if (is_key_pressed(im_key_id::tab)) {
    if (ctx->next_focused_widget_id != 0) {
      ctx->prev_focused_widget_id = ctx->focused_widget_id;
      ctx->focused_widget_id = ctx->next_focused_widget_id;
    } else {
      ctx->focused_widget_id = 0;
    }
  }
  ctx->next_focused_widget_id = 0;

  ctx->renderer.begin_frame();
}

void render() {
  auto const ctx = get_context();
  assert(ctx);

  ctx->renderer.end_frame();
  ctx->renderer.present();
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
  return ctx->layout_stack.front().bounds;
}

void set_default_color(im_color_id id, im_color color) {
  assert(id < im_color_id::last);
  auto const index = static_cast<std::size_t>(id);

  auto const ctx = get_context();
  ctx->colors[index] = color;
}

void push_color(im_color_id id, im_color color) {
  assert(id < im_color_id::last);
  auto const index = static_cast<std::size_t>(id);

  auto const ctx = get_context();
  ctx->color_state_stack.emplace_back(id, std::exchange(ctx->colors[index], color));
}

void pop_color(std::size_t cnt) {
  auto const ctx = get_context();

  while (ctx->color_state_stack.size() > 0 && cnt > 0) {
    auto const index = static_cast<std::size_t>(ctx->color_state_stack.back().id);
    ctx->colors[index] = ctx->color_state_stack.back().color;
    ctx->color_state_stack.pop_back();
  }
}

void push_id(std::string_view value) {
  get_context()->hash_stack.emplace_back(hash(value, get_context()->hash_stack.back()));
}

void push_id(int value) {
  get_context()->hash_stack.emplace_back(hash(value, get_context()->hash_stack.back()));
}

void pop_id() {
  if (auto& stack = get_context()->hash_stack; stack.size() > 1) [[likely]] {
    stack.pop_back();
  }
}

void label(std::string_view text) {
  auto const ctx = get_context();

  layout_add_widget_item(1);

  auto& widget = ctx->widget;
  widget.visible = !ctx->renderer.get_clip_rect().intersection(widget.bounds).empty();

  auto const style = im_style(get_color(im_color_id::fg), get_color(im_color_id::bg));
  ctx->renderer.draw_text_in_rect(widget.bounds, utf8_to_unicode(text), style, im_halign::left, im_valign::top);
}

auto button(std::string_view text) -> bool {
  auto const ctx = get_context();

  layout_add_widget_item(1);

  auto& widget = ctx->widget;
  widget.visible = !ctx->renderer.get_clip_rect().intersection(widget.bounds).empty();

  auto const [label, id_str] = get_content_and_id(text);

  widget.widget_id = get_id(id_str);
  widget.hovered = widget.bounds.contains(ctx->input.mouse.pos);
  widget.focused = (ctx->focused_widget_id == widget.widget_id);
  widget.pressed = false;

  if (widget.focused && (is_key_pressed(im_key_id::space) || is_key_pressed(im_key_id::enter))) {
    widget.pressed = true;
  }
  if (widget.hovered && is_mouse_pressed(im_mouse_button_id::left)) {
    widget.pressed = true;
    widget.focused = true;
  }
  if (widget.focused) {
    ctx->next_focused_widget_id = 0;
  } else {
    if (ctx->next_focused_widget_id == 0) {
      ctx->next_focused_widget_id = widget.widget_id;
    }
  }

  if (widget.visible) {
    auto const style = [&]() -> im_style {
      if (widget.hovered) {
        return im_style(get_color(im_color_id::btn_fg_hov), get_color(im_color_id::btn_bg_hov));
      }
      if (widget.focused) {
        return im_style(get_color(im_color_id::btn_fg_act), get_color(im_color_id::btn_bg_act));
      }
      return im_style(get_color(im_color_id::btn_fg), get_color(im_color_id::btn_bg));
    }();

    ctx->renderer.draw_text_in_rect(widget.bounds, utf8_to_unicode(label), style, im_halign::center, im_valign::center);
  }

  return widget.pressed;
}

} // namespace xxx
