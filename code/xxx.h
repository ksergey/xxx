// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <source_location>
#include <span>
#include <string>
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
  page_up,
  page_down,
  back_tab, // shift-tab
  last
};

/// Mouse button id
enum class im_mouse_button_id { left, middle, right, last };

/// Color id
enum class im_color_id {
  text,
  background,
  border,
  view_inactive_border,
  view_inactive_title,
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

/// How many times key was pressed since last frame (auto-repeat, slow frame)
[[nodiscard]] auto key_press_count(im_key_id id) -> int;

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

/// Place next widget at the same line
void same_line();

/// Width of the next widget (text_input, list): > 0 cells, fill(n) - rest of the line minus n cells
void set_next_item_width(int width);

/// Give keyboard focus to widget with \c label (activates its view, scrolls it into view).
/// Id is resolved like widget ids: call in the same scope (view, push_id) as the widget.
/// Takes effect on the next frame; request is dropped if no such widget is built
/// during this or the next frame. Ignored for widgets outside an open popup.
void set_focus(std::string_view label);

/// Give keyboard focus to the next widget (e.g. text input when a form opens)
void set_focus_next();

/// True if the last built widget has keyboard focus
[[nodiscard]] auto is_item_focused() -> bool;

/// Push id scope: same labels inside different scopes don't collide (e.g. widgets built in a loop)
void push_id(std::string_view id);
/// @overload
void push_id(int id);
/// Pop id scope
void pop_id();

// -----------------------------------------
// View
// -----------------------------------------

constexpr auto im_view_flag_border = int(1 << 0);
constexpr auto im_view_flag_title = int(1 << 1);
constexpr auto im_view_flags_default = im_view_flag_border | im_view_flag_title;

/// Size meaning "the rest of available space minus \c n":
///   view_begin(...) height - up to the bottom, leaving n rows (e.g. for a footer)
///   layout_row_push(...) - rest of the row, leaving n columns
///   set_next_item_width(...) - rest of the line, leaving n cells
[[nodiscard]] constexpr auto fill(int n = 0) noexcept -> int {
  return -1 - (n > 0 ? n : 0);
}

/// Begin view
/// flags:
///   im_view_flag_border - draw border around view
///   im_view_flag_title - show view name with shortcut (if set)
/// theme:
///   view_border - border color when im_view_flag_border is set
///   view_active_border - border color when im_view_flag_border is set and view is active
///   view_title - title color when im_view_flag_title is set
///   view_active_title - title color when im_view_flag_title is set and view is active
/// height (including border and title):
///   0 - fit content (default), never scrolls
///   > 0 - fixed number of rows
///   fill(n) - up to the bottom of available area minus n rows
/// View with fixed or fill height scrolls when content doesn't fit:
///   page_up / page_down when view is active, mouse wheel when hovered,
///   and automatically to a widget receiving focus.
void view_begin(std::string_view name, int flags, im_key_id shortcut = im_key_id(), int height = 0);

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

// -----------------------------------------
// Popup (modal)
// -----------------------------------------

/// Open modal popup with \c id (e.g. on button press). One popup is open at a time.
/// While open: views are inactive, input goes only to the popup, Esc closes it.
void open_popup(std::string_view id);

/// Begin popup: draws centered on top of everything, height fits content.
/// Call outside of views. When true is returned, build content and call popup_end().
/// theme: view_active_border, view_active_title, background
auto popup_begin(std::string_view id, std::string_view title, int width = 40) -> bool;

/// End popup
void popup_end();

/// Close open popup; keyboard focus returns where it was before opening
void close_popup();

/// True when any popup is open
[[nodiscard]] auto is_popup_open() -> bool;

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

/// text_input flags
constexpr auto im_input_flag_password = int(1 << 0); // show '*' instead of chars

/// Widget: tab bar " one  two  three ", selected tab highlighted
/// Left / Right switch tabs when focused (wrapping), click selects.
/// @param label is used only as widget id
/// @return true when selection changed this frame
auto tabs(std::string_view label, std::span<std::string_view const> items, int& selected) -> bool;

/// @overload
auto tabs(std::string_view label, std::span<std::string const> items, int& selected) -> bool;

/// Widget: checkbox "[x] label"
/// Toggled by space / enter when focused or by mouse click.
/// @return true when value was toggled this frame
/// theme: button_* colors
auto checkbox(std::string_view label, bool& value) -> bool;

/// Widget: list of selectable items, spans layout width
/// Up / Down / Home / End move selection when focused, click selects.
/// @param label is used only as widget id
/// @param selected is index of selected item, -1 for none
/// @param height is number of visible rows, 0 - all items; list scrolls to keep selection visible
/// @return true when item is activated: enter or click
/// theme: text, background; selected row: button_active_text (focused) / button_inactive_text, reversed
auto list(std::string_view label, std::span<std::string_view const> items, int& selected, int height = 0) -> bool;

/// @overload
auto list(std::string_view label, std::span<std::string const> items, int& selected, int height = 0) -> bool;

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
/// One cell holds 2x4 "pixels" (braille dots), so canvas takes ceil(w / 2) x ceil(h / 4) cells.
/// @param p_size is canvas size in "pixels"
/// @return true on drawing started (widget is visible); call canvas_end() only in that case
auto canvas_begin(im_vec2 p_size) -> bool;

/// End canvas drawing
void canvas_end();

/// Draw point on canvas
/// @param p_pos is pos in "pixels"
/// @param color is "pixel" color
void canvas_point(im_vec2 p_pos, im_color color = {});

} // namespace xxx
