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
    bool active = false;
    bool pressed = false;
  } widget;

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

  // elapsed seconds since last new_frame(...)

  im_clock::time_point last_frame_time;
  float elapsed = 0.0;
};

inline im_context* g_ctx = nullptr;

} // namespace xxx
