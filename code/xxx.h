// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <optional>
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
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    last
};

/// Mouse button id
enum class im_mouse_button_id { left, middle, right, last };

/// Terminal input event (see terminal.h): what the terminal reported, in order
struct im_event {
    enum class type : std::uint8_t {
        key,         // key, mods; Space is reported as text ' ' followed by key space
        text,        // ch (one codepoint), mods (im_mod_alt: a shortcut, not text)
        mouse_move,  // pos
        mouse_press, // button, pos
        mouse_wheel, // wheel (> 0 down), pos
        resize       // pos: new size in cells
    };
    type kind = type::key;
    im_key_id key = im_key_id();
    std::uint32_t ch = 0;
    std::uint8_t mods = 0;
    im_mouse_button_id button = im_mouse_button_id::left;
    im_vec2 pos = im_vec2();
    int wheel = 0;
};

/// Text attributes, combined with |
constexpr auto im_attr_bold = std::uint32_t(1 << 0);
constexpr auto im_attr_dim = std::uint32_t(1 << 1);
constexpr auto im_attr_italic = std::uint32_t(1 << 2);
constexpr auto im_attr_underline = std::uint32_t(1 << 3);
constexpr auto im_attr_blink = std::uint32_t(1 << 4);
constexpr auto im_attr_reverse = std::uint32_t(1 << 5);
constexpr auto im_attr_strikeout = std::uint32_t(1 << 6);

/// Theme roles: what a style means, not which widget draws it
enum class im_role : std::uint8_t {
    text,   // regular text and the background of everything
    muted,  // secondary: unfocused buttons and tabs, hints
    accent, // highlight: focused button brackets and prompt, spinner, progress
    border, // view border, button brackets
    border_focused,
    title, // view title
    title_focused,
    focus,     // focused item: button label, checkbox box, list row, tab, text cursor
    selection, // selected item of an unfocused list / table / tabs
    header,    // table header
    input,     // text input
    input_focused,
    placeholder, // text input placeholder
    error,       // for applications: label("...", im_role::error)
    warning,
    success,
    last
};

/// Style of a role. Unset colors are taken from im_role::text (so a theme background applies everywhere)
struct im_role_style {
    std::optional<im_color> fg;
    std::optional<im_color> bg;
    std::uint32_t attrs = 0;
};

/// Built-in themes
enum class im_theme_preset {
    terminal, // default: 16 colors of the user's terminal scheme, works on light and dark backgrounds
    classic   // fixed 24-bit palette for dark backgrounds
};

class terminal;

/// Init library on a terminal owned by the application (terminal.h): the application waits on its
/// descriptors in its own event loop; process_input_events() then never waits, it takes queued events.
/// The terminal must outlive shutdown().
void init(terminal& term);

/// Init library on the terminal (xterm compatible terminals).
/// Environment: ESCDELAY=ms - how long a lone Esc waits for the rest of a key sequence (default 25)
void init();

/// Shutdown library
void shutdown();

/// Key modifiers, combined with | (bits as xterm encodes them)
constexpr auto im_mod_shift = int(1 << 0);
constexpr auto im_mod_alt = int(1 << 1);
constexpr auto im_mod_ctrl = int(1 << 2);

/// Update internal state: take pending input without waiting
void process_input_events();

/// Wait for \c timeout (or until first input) and update internal state.
/// Event driven main loop: build a frame only when something happened, instead of busy redraw:
///   while (true) {
///     process_input_events(running ? time_to_next_update : xxx::wait_forever);
///     ...build ui, render()...
///   }
/// @return true if any event (key, mouse, resize) arrived, false on timeout
auto process_input_events(std::chrono::milliseconds timeout) -> bool;

/// Timeout for process_input_events(...): wait until input arrives
inline constexpr auto wait_forever = std::chrono::milliseconds(-1);

/// Thread-safe: wake up process_input_events(...) waiting in another thread, e.g. when a background
/// thread has new data to show. The woken call returns true. Call only between init() and shutdown().
void wake_up();

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

/// True if key was pressed with exactly these modifiers (e.g. arrow_right, im_mod_ctrl).
/// is_key_pressed(id) is true for any modifiers.
[[nodiscard]] auto is_key_pressed(im_key_id id, int mods) -> bool;

/// True if the character was typed (without Alt) this frame. Shortcuts with plain letters
/// are safe when no text input has focus: check wants_text_input() first.
[[nodiscard]] auto is_char_pressed(char32_t ch) -> bool;

/// True if Alt + character was pressed (e.g. 'x'; Alt+Shift+x gives 'X').
/// Such presses never reach text inputs: safe shortcuts even while typing.
[[nodiscard]] auto is_alt_pressed(char32_t ch) -> bool;

/// Replace the whole theme with a built-in one
void use_theme(im_theme_preset preset);

/// Set style of a role in the theme
void set_style(im_role role, im_role_style const& style);

/// Current style of a role (overrides from push_style applied), unset colors resolved
[[nodiscard]] auto get_style(im_role role) -> im_role_style;

/// Style of a role as set in the theme: unset colors stay unset (they follow im_role::text).
/// Use it to change a part of a style and set it back without freezing the inherited colors.
[[nodiscard]] auto get_theme_style(im_role role) -> im_role_style;

/// Override style of a role for the widgets that follow, until pop_style
void push_style(im_role role, im_role_style const& style);

/// Undo the last \c count push_style calls
void pop_style(std::size_t count = 1);

/// push_style for a scope: the style is restored when the object goes away
///   { auto const s = xxx::scoped_style(xxx::im_role::text, {.fg = xxx::ansi::red}); xxx::label("lost"); }
class [[nodiscard]] scoped_style {
public:
    scoped_style(im_role role, im_role_style const& style) {
        push_style(role, style);
    }
    ~scoped_style() {
        pop_style();
    }
    scoped_style(scoped_style const&) = delete;
    scoped_style& operator=(scoped_style const&) = delete;
};

/// Begin row layout
/// @param columns is number of columns in row
void layout_row_begin(std::size_t columns);

/// Size meaning "the rest of available space minus \c n":
///   view_begin(...) height - up to the bottom, leaving n rows (e.g. for a footer)
///   layout_row_push(...) - rest of the row, leaving n columns
///   set_next_item_width(...) - rest of the line, leaving n cells
[[nodiscard]] constexpr auto fill(int n = 0) noexcept -> int {
    return -1 - (n > 0 ? n : 0);
}

/// Width of a layout column (layout_row_push) or a table column (im_table_column)
struct im_width {
    enum class unit : std::uint8_t {
        cells, // fixed number of cells
        ratio, // part of the available width, 0..1
        fill   // the rest of the width, minus `value` cells
    };
    unit kind = unit::fill;
    float value = 0.0f;

    constexpr im_width() noexcept = default;
    constexpr im_width(unit k, float v) noexcept : kind(k), value(v) {}

    /// Number form, kept for compatibility: > 1 cells, (0, 1] ratio, fill(n) the rest minus n.
    /// Note: 1 means 100% of the width, not one cell. Prefer cells(n) / ratio(r) / fill(n).
    constexpr im_width(double v) noexcept
        : kind(v < 0.0   ? unit::fill
               : v > 1.0 ? unit::cells
                         : unit::ratio),
          value(float(v < 0.0 ? -v - 1.0 : v)) {}
    constexpr im_width(float v) noexcept : im_width(double(v)) {}
    constexpr im_width(int v) noexcept : im_width(double(v)) {}
};

/// Width of exactly n cells
[[nodiscard]] constexpr auto cells(int n) noexcept -> im_width {
    return im_width(im_width::unit::cells, float(n > 0 ? n : 0));
}

/// Width as a part of the available width, 0..1
[[nodiscard]] constexpr auto ratio(float r) noexcept -> im_width {
    return im_width(im_width::unit::ratio, r < 0.0f ? 0.0f : r > 1.0f ? 1.0f : r);
}

/// Push column of the given width: cells(n), ratio(r), fill(n)
void layout_row_push(im_width width);

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

/// Put utf8 text into system clipboard via OSC 52 terminal escape sequence.
/// Works over ssh; inside tmux needs "set -g set-clipboard on" (or allow-passthrough). Terminal may ignore it.
/// Widgets copy with ctrl-c: text_input (whole text, never for passwords), list / table (selected row).
void set_clipboard(std::string_view text);

/// Number of ids used by more than one widget or view in the last rendered frame.
/// Colliding widgets share focus and state (e.g. two buttons "ok" in one view);
/// fix with push_id(...) or "label##key".
[[nodiscard]] auto id_collision_count() -> int;

/// Mark colliding widgets with red '!' on screen. Default: on in debug builds (no NDEBUG).
void show_id_collisions(bool show);

/// True when a text input has keyboard focus: typed characters go there.
/// Otherwise an application can safely use plain letters and digits as shortcuts.
[[nodiscard]] auto wants_text_input() -> bool;

/// Describe an application key for the built-in help (F1, or ? when no text input is focused).
/// Call every frame, e.g. key_hint("c-q", "quit")
void key_hint(std::string_view keys, std::string_view action);

/// Built-in help popup: application keys (key_hint), view shortcuts and navigation keys. On by default
void enable_help(bool enabled);

/// Esc goes back to the previously active view (when no popup is open). On by default;
/// turn off if the application uses Esc itself. Esc is consumed only when it went back.
void enable_esc_back(bool enabled);

/// Widget: one line of keys for the focused widget (e.g. at the bottom of the screen), \c extra is appended
void key_hints(std::string_view extra = {});

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

/// Label drawn in the style of a role, e.g. label("connection lost", im_role::error)
void label(std::string_view text, im_role role);

/// Label with its own style, e.g. label("12 ms", {.fg = ansi::green}); unset colors come from im_role::text
void label(std::string_view text, im_role_style const& style);

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

/// Text alignment (table column, label, view title)
enum class im_align { left, center, right };

/// Frame of views and popups
enum class im_border : std::uint8_t {
    rounded,    // ╭─╮ (default)
    plain,      // ┌─┐
    thick,      // ┏━┓
    double_line // ╔═╗
};

/// How views look: frame of inactive views, frame of the active view (and popups), where titles go
void set_view_style(im_border inactive, im_border active, im_align title_align = im_align::center);

/// Part of a label with its own style, e.g. a key in bold in a help line
struct im_text {
    std::string_view text;
    im_role role = im_role::text;
    std::uint32_t attrs = 0; // added to the role's attributes, e.g. im_attr_bold
};

/// Label made of parts; with center / right alignment it takes the whole width
void label(std::span<im_text const> parts, im_align align = im_align::left);
/// @overload
void label(std::initializer_list<im_text> parts, im_align align = im_align::left);

/// j / k move the selection in lists and tables, g / G go to the first / last row (vim style). Off by default
void enable_vim_keys(bool enabled);

/// Table column
struct im_table_column {
    std::string_view title;
    im_width width = fill(); // cells(n), ratio(r) of table width, fill(n): the rest, shared by fill columns
    im_align align = im_align::left;
};

/// Widget: table with underlined header; rows are selectable like in list(...)
/// @param cells are row-major: rows * columns.size() items
/// @param height is number of visible data rows (header excluded), 0 - all rows
/// @return true when row is activated: enter or click
/// theme: text, background; header: button_inactive_fx; selected row like in list(...)
auto table(std::string_view label, std::span<im_table_column const> columns, std::span<std::string_view const> cells,
    int& selected, int height = 0) -> bool;

/// @overload
auto table(std::string_view label, std::span<im_table_column const> columns, std::span<std::string const> cells,
    int& selected, int height = 0) -> bool;

/// More table options
struct im_table_options {
    /// role of each row (text when empty or shorter), e.g. muted for unavailable items; the selected row
    /// keeps the focus / selection style
    std::span<im_role const> row_roles = {};
    /// blank line between the header and the rows
    bool header_gap = false;
};

/// @overload
auto table(std::string_view label, std::span<im_table_column const> columns, std::span<std::string const> cells,
    int& selected, int height, im_table_options const& options) -> bool;
/// @overload
auto table(std::string_view label, std::span<im_table_column const> columns, std::span<std::string_view const> cells,
    int& selected, int height, im_table_options const& options) -> bool;

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
