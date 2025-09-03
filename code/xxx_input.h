// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include "xxx_internal.h"

namespace xxx::v2 {

void add_key_event(im_key_id id);
void add_mouse_pos_event(int x, int y);
void add_mouse_button_event(im_mouse_button_id id, int x, int y);
void add_input_character(std::uint32_t ch);
void add_input_characters_utf8(char const* str);
void clear_input_keys();
void clear_input_mouse();

} // namespace xxx::v2
