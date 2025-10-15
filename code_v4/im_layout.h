// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <cmath>

#include "im_rect.h"
#include "im_stack.h"
#include "im_vec2.h"

namespace xxx {

enum class im_layout_type {
  container, // layout is fixed size and fixed pos
  column,    // layout is column of a row
  row        // layout is row
};

struct im_layout_data_none {};

struct im_layout_data_container {
  int border;
};

struct im_layout_data_row {
  int columns;      // columns count inside row
  int index;        // current column index
  int cursor_max_y; // track max y over all columns on row
};

struct im_layout_state {
  im_layout_type type;
  // layout bounds (global)
  //   rect.min - layout position
  //   rect.max.x != rect.min.x -> fixed width
  //   rect.max.y != rect.min.y -> fixed height
  im_rect rect;

  union {
    im_layout_data_none none = {};
    im_layout_data_container container;
    im_layout_data_row row;
  };
};

struct im_layout {
  im_stack<im_layout_state> layout_state_stack = im_stack<im_layout_state>(32);
  im_stack<im_vec2> cursor_state_stack = im_stack<im_vec2>(32);
  im_vec2 cursor = im_vec2(0, 0);

  struct {
    im_rect rect;
  } widget_item;

  // Reserve space for widget item
  auto add_widget_item(int height) noexcept -> im_rect {
    if (height <= 0) [[unlikely]] {
      assert(false && "im_layout::add_widget_item(...) negative or zero height");
      return im_rect();
    }
    auto& layout = layout_state_stack.back();
    if (layout.type != im_layout_type::container && layout.type != im_layout_type::column) {
      assert(false && "im_layout::add_widget_item(...) unexpected layout type");
      return im_rect();
    }

    widget_item.rect.min = cursor;
    widget_item.rect.max = im_vec2(layout.rect.max.x, cursor.y + height - 1);
    cursor.y += height;

    return widget_item.rect;
  }

  void reset(im_rect const& rect) {
    layout_state_stack.clear();
    cursor_state_stack.clear();

    auto& layout = layout_state_stack.emplace_back();
    layout.type = im_layout_type::container;
    layout.rect = rect;
    layout.container = im_layout_data_container{.border = 0};

    cursor = layout.rect.min;
  }
};

} // namespace xxx
