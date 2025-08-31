// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <array>
#include <vector>

#include <termbox2.h>

#include "xxx2.h"

namespace xxx {

static_assert(std::is_same_v<uintattr_t, std::uint64_t>, "termbox2 invalid configuration");

// ------------------------------------
// common
// ------------------------------------

// clock type
using clock = std::chrono::steady_clock;

// ------------------------------------
// layout
// ------------------------------------
enum class im_layout_type { container, column, row };

struct im_layout {
  im_layout_type type = im_layout_type::container;
  im_vec2 pos;
  im_vec2 size;
  im_vec2 filled;
  std::size_t columns;
  std::size_t column;
};

// ------------------------------------
// context
// ------------------------------------
struct im_context {
  std::vector<im_layout> layouts_stack;
};

} // namespace xxx
