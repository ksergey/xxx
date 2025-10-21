// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <termbox2.h>

#include "xxx.h"

#include "im_hash_id.h"
#include "im_input.h"
#include "im_layout.h"
#include "im_renderer.h"
#include "im_stack.h"
#include "im_theme.h"

namespace xxx {

static_assert(std::is_same_v<uintattr_t, std::uint64_t>, "termbox2 invalid configuration");

struct im_context {
  im_input input;
  im_hash_id hash_id;
  im_theme theme;
  im_layout layout;
  im_renderer renderer;

  struct {
    std::string current_title;
    im_id current_id = im_id();
    int current_flags = 0;
    im_id active_id = im_id();
    im_id force_next_id = im_id();
    bool active;
  } view;

  struct {
    im_id current_id = im_id();
    im_id active_id = im_id();
    im_id first_id = im_id();
    im_id next_id = im_id();
    bool active;
    bool pressed;
  } widget;

  struct {
    im_id active_id = im_id();
    std::vector<std::uint32_t> text;
    int cursor_pos = 0;
    int scroll_offset = 0;
  } text_input;

  struct {
    int width = 0;
    int height = 0;
    im_rect rect;
    std::vector<im_cell> buffer;
  } canvas;
};

im_context* g_ctx = nullptr;

} // namespace xxx
