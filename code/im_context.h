// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


#include "xxx.h"

#include "im_allocator.h"
#include "im_backend.h"
#include "im_hash_id.h"
#include "im_input.h"
#include "im_layout.h"
#include "im_renderer.h"
#include "im_stack.h"
#include "im_theme.h"

namespace xxx {


struct im_context {
  // declared first: destroyed last, after everything that may reference terminal state
  std::unique_ptr<im_backend> backend;

  im_allocator allocator;

  im_input input;
  im_hash_id hash_id;
  im_theme theme;
  im_layout layout;
  im_renderer renderer;

  // persistent per-view scroll state
  struct view_scroll {
    int offset = 0;          // first visible content row
    int content_height = 0;  // measured on previous frame
    im_id focus_id = im_id(); // last widget scrolled into view
    int focusable = 0;        // focusable widgets on previous frame (none: arrows scroll by line)
  };

  struct {
    std::string current_title;
    im_id current_id = im_id();
    int current_flags = 0;
    im_id active_id = im_id();
    im_id force_next_id = im_id();
    bool active = false;

    // current view geometry, valid between view_begin and view_end
    bool current_bounded = false; // fixed or fill height
    int current_bottom = 0;       // last row of view (including border)
    im_rect current_viewport;     // visible content area
    int current_content_top = 0;  // y of first content row (scrolled)
    view_scroll* current_scroll = nullptr;
    int current_focusable = 0; // focusable widgets in current view so far

    std::unordered_map<im_id, view_scroll> scroll;

    // focus history: Esc goes back; returning to a view restores its focused widget
    std::vector<im_id> history;
    std::unordered_map<im_id, im_id> last_widget;
    bool esc_back = true;

    // look: frames of inactive / active views (and popups), title placement
    im_border border_inactive = im_border::rounded;
    im_border border_active = im_border::rounded;
    im_align title_align = im_align::center;
  } view;

  struct {
    im_id current_id = im_id();
    im_id active_id = im_id();
    im_id first_id = im_id();
    im_id next_id = im_id();
    im_id prev_id = im_id(); // focusable right before active one (shift-tab)
    im_id last_id = im_id(); // last focusable of active view (shift-tab wrap)
    bool active = false;
    bool pressed = false;

    // focus request: set_focus(...) / set_focus_next(), matched while building frame,
    // applied by process_input_events() of the next frame
    im_id focus_request_id = im_id();
    int focus_request_age = 0;    // frames since request, dropped when > 1
    bool focus_next = false;      // set_focus_next(): next focusable widget is the target
    struct {
      im_id id = im_id();
      im_id view_id = im_id();
      bool in_popup = false;
    } focus_target;

    // widget clicked by mouse this frame (hit-tested against previous frame)
    im_id clicked_id = im_id();
    im_vec2 clicked_pos;
  } widget;

  // visible (clipped) areas recorded while building a frame, hit-tested by
  // process_input_events() of the next frame: clicks land on what user saw
  struct hit_item {
    im_id id;
    im_id view_id;
    im_rect rect;
  };
  std::vector<hit_item> view_hits;
  std::vector<hit_item> widget_hits;

  // arrow keys navigation: focus moves to the nearest item in the direction of an arrow
  // the focused widget doesn't use. Recorded while building a frame, used by the next one.
  enum nav_keys : std::uint8_t {
    nav_none = 0,
    nav_up = 1 << 0,
    nav_down = 1 << 1,
    nav_left = 1 << 2,
    nav_right = 1 << 3,
    nav_vertical = nav_up | nav_down,
    nav_horizontal = nav_left | nav_right,
  };
  struct nav_item {
    im_id id;          // widget, or view itself for a scrollable view without widgets
    im_id view_id;
    im_rect rect;      // full rect, also when scrolled out of the view
    bool visible;      // at least partly visible
    std::uint8_t keys; // arrows the item uses itself
  };
  std::vector<nav_item> nav_items;

  struct {
    im_id active_id = im_id();
    std::vector<std::uint32_t> text;
    int cursor_pos = 0;
    int scroll_offset = 0; // in terminal columns
    std::vector<std::uint32_t> kill_buffer; // last killed text (ctrl-w/u/k), yanked by ctrl-y
  } text_input;

  struct {
    im_rect rect;
    im_vec2 size;
    std::span<im_cell> data;
  } canvas;

  struct {
    im_id open_id = im_id();
    im_id saved_widget_id = im_id();           // focus to restore on close
    std::unordered_map<im_id, int> heights;    // content height measured on previous frame

    // valid between popup_begin and popup_end
    im_id current_id = im_id();
    std::string current_title;
    im_rect current_rect; // outer rect, bottom is fixed in popup_end
    bool measuring = false; // first frame: height unknown, layers are not shown
    bool close_requested = false;
    int saved_layer = 0;
    im_vec2 saved_cursor;
    int saved_last_cursor_y = 0;
    bool saved_same_line = false;
  } popup;

  // id collision detector: ids registered while building a frame, checked in render()
  struct registered_id {
    im_id id;
    im_rect visible; // clipped area for the marker, may be empty
  };
  std::vector<registered_id> frame_ids;
  std::vector<registered_id> frame_ids_sorted; // scratch, keeps capacity
  int id_collisions = 0;
#ifdef NDEBUG
  bool show_id_collisions = false;
#else
  bool show_id_collisions = true;
#endif

  // something was deferred to the next frame (popup measuring, view switch, focus, scroll):
  // render() wakes the event loop, so it doesn't wait for user input to show it
  bool next_frame_requested = false;

  // width for the next widget, 0 - widget default (see set_next_item_width)
  int next_item_width = 0;

  // kind of the widget with keyboard focus: drives key_hints() and wants_text_input()
  enum class widget_kind : std::uint8_t { none, button, checkbox, text_input, password, list, table, tabs, scroll };
  widget_kind focused_kind = widget_kind::none;      // this frame, so far
  widget_kind last_focused_kind = widget_kind::none; // previous frame

  // built-in help
  struct {
    bool enabled = true;
    std::vector<std::pair<std::string, std::string>> app_keys;  // key_hint() this frame
    std::vector<std::pair<std::string, std::string>> view_keys; // view shortcuts this frame
    int selected = 0;
  } help;

  bool vim_keys = false; // j / k / g / G in lists and tables

  // persistent first visible row per list widget
  std::unordered_map<im_id, int> list_scroll;

  // elapsed seconds since last new_frame(...)

  im_clock::time_point last_frame_time;
  float elapsed = 0.0;
};

inline im_context* g_ctx = nullptr;

} // namespace xxx
