// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx_internal.h"

#include <cassert>

namespace xxx::v2 {

void input_add_key_event(im_key_id id) {
  assert(id < im_key_id::last);

  auto const ctx = get_context();
  assert(ctx);

  auto& keyboard = ctx->input.keyboard;
  keyboard.keys[static_cast<std::size_t>(id)].clicked++;
}

void input_add_mouse_pos_event(int x, int y) {
  auto const ctx = get_context();
  assert(ctx);

  auto& mouse = ctx->input.mouse;
  mouse.pos = im_vec2{x, y};
  mouse.delta = mouse.pos - mouse.prev;
}

void input_add_mouse_button_event(im_mouse_button_id id, int x, int y) {
  assert(id < im_mouse_button_id::last);

  auto const ctx = get_context();
  assert(ctx);

  auto& mouse = ctx->input.mouse;
  auto& button = mouse.buttons[static_cast<std::size_t>(id)];
  button.clicked++;
  button.clicked_pos = im_vec2{x, y};
  mouse.delta = im_vec2{0, 0};
}

void input_add_character(std::uint32_t ch) {
  auto const ctx = get_context();
  assert(ctx);

  auto& keyboard = ctx->input.keyboard;
  if (keyboard.text_length < keyboard.text.size()) {
    keyboard.text[keyboard.text_length++] = ch;
  }
}

void input_add_characters_utf8(char const* str) {
  for (auto const ch : utf8_to_unicode(str)) {
    input_add_character(ch);
  }
}

void input_clear_keys() {
  auto const ctx = get_context();
  assert(ctx);

  auto& keyboard = ctx->input.keyboard;
  keyboard.keys.fill(im_key_state{.clicked = 0});
  keyboard.text_length = 0;
}

void input_clear_mouse() {
  auto const ctx = get_context();
  assert(ctx);

  auto& mouse = ctx->input.mouse;
  mouse.buttons.fill(im_mouse_button_state{.clicked = 0, .clicked_pos = im_vec2{0, 0}});
  mouse.prev = mouse.pos;
  mouse.delta = im_vec2{0, 0};
}

} // namespace xxx::v2
