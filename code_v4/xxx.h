// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

#include "im_color.h"
#include "im_rect.h"
#include "im_vec2.h"

namespace xxx {

// keyboard key id
enum class im_key_id { tab, enter, esc, space, quit, last };

// mouse button id
enum class im_mouse_button_id { left, middle, right, last };

// helper: color id
enum class im_color_id { text, background, border, border_active, last };

// init library
void init();

// shutdown library
void shutdown();

// update internal state
void process_input_events();

// start drawing new frame
void new_frame();

// render frame
void render();

void debug();

// get terminal screen rect
[[nodiscard]] auto get_screen_rect() -> im_rect;

// check key pressed
[[nodiscard]] auto is_key_pressed(im_key_id id) -> bool;

// set default color
void set_default_color(im_color_id id, im_color color);

// save current color and set new
void push_color(im_color_id id, im_color color);

// pop color state
void pop_color(std::size_t cnt = 1);

void layout_row_begin(std::size_t columns);

void layout_row_push(float ratio_or_width);

void layout_row_end();

// -----------------------------------------
// View
// -----------------------------------------

void view_begin(std::string_view name);

void view_end();

[[nodiscard]] auto view_get_name() -> std::string_view;

[[nodiscard]] auto view_is_active() -> bool;

// -----------------------------------------
// Widgets
// -----------------------------------------

void label(std::string_view text);

} // namespace xxx
