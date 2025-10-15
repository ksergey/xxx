// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

#include "im_color.h"
#include "im_rect.h"
#include "im_vec2.h"

namespace xxx {

// keyboard key id
enum class im_key_id {
  backspace = 1, // ctrl-h
  backspace2,
  del,
  tab,   // ctrl-i
  enter, // ctrl-m
  space,
  esc,
  home,
  end,
  arrow_up,
  arrow_down,
  arrow_left,
  arrow_right,
  ctrl_a,
  ctrl_b,
  ctrl_c,
  ctrl_d,
  ctrl_e,
  ctrl_f,
  ctrl_g,
  // ctrl_h,
  // ctrl_i,
  ctrl_j,
  ctrl_k,
  // ctrl_m,
  ctrl_n,
  ctrl_o,
  ctrl_p,
  ctrl_q,
  ctrl_r,
  ctrl_s,
  ctrl_t,
  ctrl_u,
  ctrl_v,
  ctrl_w,
  ctrl_x,
  ctrl_y,
  ctrl_z,
  last
};

// mouse button id
enum class im_mouse_button_id { left, middle, right, last };

// helper: color id
enum class im_color_id {
  text,
  background,
  border,
  view_border,
  view_title,
  view_active_border,
  view_active_title,
  button_label,
  button_active_label,
  input_text,
  input_background,
  input_active_text,
  input_active_background,
  input_placeholder,
  last
};

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

constexpr auto im_view_flag_border = int(1 << 0);
constexpr auto im_view_flag_title = int(1 << 1);

// begin view
// flags:
//   im_view_flag_border - draw border around view
//   im_view_flag_title - show view name with shortcut (if set)
// theme:
//   view_border - border color when im_view_flag_border is set
//   view_active_border - border color when im_view_flag_border is set and view is active
//   view_title - title color when im_view_flag_title is set
//   view_active_title - title color when im_view_flag_title is set and view is active
void view_begin(std::string_view name, int flags, im_key_id shortcut = im_key_id());

// overload
inline void view_begin(std::string_view name, im_key_id shortcut = im_key_id()) {
  return view_begin(name, im_view_flag_border | im_view_flag_title, shortcut);
}

// end view
void view_end();

// -----------------------------------------
// Widgets
// -----------------------------------------

// begin panel
void panel_begin();
// end panel
void panel_end();

// widget: label
// theme
//   text - label color
void label(std::string_view text);

// widget: button
// return true on pressed
// theme:
//   button_label - button label color
//   button_active_label - button label color when widget active
auto button(std::string_view label) -> bool;

// widget: text_input
// return true on text entered
auto text_input(std::string_view placeholder, std::string& input, int flags = 0) -> bool;

// widget: drawing canvas
void canvas_begin(int width, int height);
void canvas_end();
void canvas_point(im_vec2 pos, im_color color = {});

} // namespace xxx
