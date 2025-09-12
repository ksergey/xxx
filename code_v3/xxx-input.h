// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include "xxx.h"

namespace xxx {

void input_add_key_event(im_key_id id);
void input_add_mouse_pos_event(im_vec2 const& pos);
void input_add_mouse_button_event(im_mouse_button_id id, im_vec2 const& pos);
void input_add_character(std::uint32_t ch);
void input_add_characters_utf8(char const* str);

// reset input internal state
void input_reset();

} // namespace xxx
