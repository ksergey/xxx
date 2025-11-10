// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <cmath>

#include "im_rect.h"
#include "im_stack.h"
#include "im_vec2.h"

#if 0
#include <print>
namespace xxx {

template <typename... Ts>
void debug(std::format_string<Ts...> fmt, Ts&&... args) {
  FILE* file = ::fopen("debug.txt", "a");
  if (!file) {
    return;
  }
  std::print(file, fmt, std::forward<Ts>(args)...);
  std::print(file, "\n");
  ::fclose(file);
}

} // namespace xxx
#endif

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
  static constexpr auto spacing = im_vec2(1, 0);

  im_stack<im_layout_state> layout_state_stack = im_stack<im_layout_state>(32);
  im_vec2 cursor = im_vec2(0, 0);
  int last_cursor_y = 0;
  bool same_line = false;

  struct {
    // current widget item bounds
    im_rect rect;
    // current widget item at the same line as previous widget item
    bool same_line = false;
  } widget_item;

  /// Reserve layout space (number of top lines)
  auto reserve_layout_lines(int height) noexcept -> im_rect {
    if (height <= 0) [[unlikely]] {
      assert(false && "im_layout::add_widget_item(...) negative or zero height");
      return im_rect();
    }
    auto& layout = layout_state_stack.back();
    if (layout.type != im_layout_type::container && layout.type != im_layout_type::column) {
      assert(false && "im_layout::add_widget_item(...) unexpected layout type");
      return im_rect();
    }

    // reset same_line flag
    same_line = false;

    widget_item.rect.min = cursor;
    widget_item.rect.max = im_vec2(layout.rect.max.x, cursor.y + height - 1);
    widget_item.same_line = false;
    cursor.y += height;

    return widget_item.rect;
  }

  /// Reserve space for widget item
  auto add_widget_item(im_vec2 const& size) noexcept -> im_rect {
    if (size.x <= 0 || size.y <= 0) [[unlikely]] {
      assert(false && "im_layout::add_widget_item(...) negative or zero width or height");
      return im_rect();
    }
    auto& layout = layout_state_stack.back();
    if (layout.type != im_layout_type::container && layout.type != im_layout_type::column) {
      assert(false && "im_layout::add_widget_item(...) unexpected layout type");
      return im_rect();
    }

    // reset layout same_line flag
    widget_item.same_line = std::exchange(same_line, false);

    if (widget_item.same_line) {
      // restore cursor y
      cursor.y = last_cursor_y;
    } else {
      // save cursor y
      last_cursor_y = cursor.y;
      // restore cursor x to start of layout
      cursor.x = layout.rect.min.x;
    }

    widget_item.rect.min = cursor;
    widget_item.rect.max = widget_item.rect.min + size - im_vec2(1, 1);

    cursor += size + spacing;

    return widget_item.rect;
  }

  void reset(im_rect const& rect) {
    layout_state_stack.clear();

    auto& layout = layout_state_stack.emplace_back();
    layout.type = im_layout_type::container;
    layout.rect = rect;
    layout.container = im_layout_data_container{.border = 0};

    cursor = layout.rect.min;
    last_cursor_y = cursor.y;
    same_line = false;
  }
};

} // namespace xxx
