// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <source_location>
#include <string_view>

#include "im_color.h"
#include "im_rect.h"
#include "im_vec2.h"

namespace xxx {
namespace detail {

template <typename T, typename Tag = void>
[[nodiscard]] auto storage_for() noexcept -> T& {
  static T value = T();
  return value;
}

} // namespace detail

/// Keyboard key ids
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

/// Mouse button id
enum class im_mouse_button_id { left, middle, right, last };

/// Color id
enum class im_color_id {
  text,
  background,
  border,
  view_border,
  view_title,
  view_active_border,
  view_active_title,
  button_inactive_background,
  button_inactive_text,
  button_inactive_fx,
  button_active_background,
  button_active_text,
  button_active_fx,
  input_inactive_background,
  input_inactive_text,
  input_inactive_prompt,
  input_active_background,
  input_active_text,
  input_active_prompt,
  input_placeholder,

  last
};

/// Init library
void init();

/// Shutdown library
void shutdown();

/// Update internal state
void process_input_events();

/// Start drawing new frame
void new_frame();

/// Render frame
void render();

// XXX: remove
void debug();

/// Get terminal screen rect
[[nodiscard]] auto get_screen_rect() -> im_rect;

/// Check key pressed
[[nodiscard]] auto is_key_pressed(im_key_id id) -> bool;

/// Set default color
void set_default_color(im_color_id id, im_color color);

/// Save current color and set new
void push_color(im_color_id id, im_color color);

/// Pop color state
void pop_color(std::size_t cnt = 1);

/// Begin row layout
/// @param columns is number of columns in row
void layout_row_begin(std::size_t columns);

/// Push column
/// @param ratio_or_width is ratio of parent layout width (in case of value < 1.0) or width in chars
void layout_row_push(float ratio_or_width);

/// End row layout
void layout_row_end();

// -----------------------------------------
// View
// -----------------------------------------

constexpr auto im_view_flag_border = int(1 << 0);
constexpr auto im_view_flag_title = int(1 << 1);

/// Begin view
/// flags:
///   im_view_flag_border - draw border around view
///   im_view_flag_title - show view name with shortcut (if set)
/// theme:
///   view_border - border color when im_view_flag_border is set
///   view_active_border - border color when im_view_flag_border is set and view is active
///   view_title - title color when im_view_flag_title is set
///   view_active_title - title color when im_view_flag_title is set and view is active
void view_begin(std::string_view name, int flags, im_key_id shortcut = im_key_id());

/// @overload
inline void view_begin(std::string_view name, im_key_id shortcut = im_key_id()) {
  return view_begin(name, im_view_flag_border | im_view_flag_title, shortcut);
}

/// End view
void view_end();

// -----------------------------------------
// Widgets
// -----------------------------------------

/// Begin panel drawing
void panel_begin();

/// End panel
void panel_end();

/// Widget: label
/// @param text is label text
///
/// theme:
///   text - label color
void label(std::string_view text);

/// Widget: button
/// @param label is widget label
/// @return true on button pressed ("enter" or "space" pressed)
///
/// theme:
///   button_inactive_background
///   button_inactive_text
///   button_inactive_fx
///   button_active_background
///   button_active_text
///   button_active_fx
auto button(std::string_view label) -> bool;

/// Widget: text input
/// @param placeholder is placeholder when input is empty
/// @param input is reference to string storagage for input
/// @return true on "enter" pressed
///
/// theme:
///   input_inactive_background
///   input_inactive_text
///   input_inactive_prompt
///   input_active_background
///   input_active_text
///   input_active_prompt
///   input_placeholder
auto text_input(std::string_view placeholder, std::string& input, int flags = 0) -> bool;

/// Widget: spinner
/// @param text is optional spinner text
/// @param step is storage for step counter
void spinner(std::string_view text, float& step);

/// @overload
/// @tparam Tag is tag for step storage
template <typename Tag = struct SpinnerDefaultTag>
void spinner(std::string_view text = {}) {
  spinner(text, detail::storage_for<float, Tag>());
}

/// Widget: progress
/// @param value is progress value ([0..100])
void progress(float const& value);

/// Begin canvas drawing
/// @param p_size is canvas size in "pixels"
/// @return true on drawing started (widget is visible)
auto canvas_begin(im_vec2 p_size) -> bool;

/// End canvas drawing
void canvas_end();

/// Draw point on canvas
/// @param p_pos is pos in "pixels"
/// @param color is "pixel" color
void canvas_point(im_vec2 p_pos, im_color color = {});

} // namespace xxx
