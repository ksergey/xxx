// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <termbox2.h>

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

static_assert(std::is_same_v<uintattr_t, std::uint64_t>, "termbox2 invalid configuration");

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

    std::unordered_map<im_id, view_scroll> scroll;
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

  struct {
    im_id active_id = im_id();
    std::vector<std::uint32_t> text;
    int cursor_pos = 0;
    int scroll_offset = 0; // in terminal columns
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

  // width for the next widget, 0 - widget default (see set_next_item_width)
  int next_item_width = 0;

  // persistent first visible row per list widget
  std::unordered_map<im_id, int> list_scroll;

  // elapsed seconds since last new_frame(...)

  im_clock::time_point last_frame_time;
  float elapsed = 0.0;
};

inline im_context* g_ctx = nullptr;

} // namespace xxx
