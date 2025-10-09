// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <string>

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
    std::string current_name; // TODO: should be there?
    im_id current_id = im_id();
    im_id active_id = im_id();
    im_id first_id = im_id();
    im_id next_id = im_id();
    bool active;
  } view;
};

im_context* g_ctx = nullptr;

} // namespace xxx
