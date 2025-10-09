// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <cmath>

#include "im_rect.h"
#include "im_stack.h"
#include "im_vec2.h"

namespace xxx {

class im_layout {
private:
  enum class layout_type {
    container, // layout is fixed size and fixed pos
    column,    // layout is column of a row
    row        // layout is row
  };

  struct layout_none {};

  struct layout_row_data {
    int columns;      // columns count inside row
    int index;        // current column index
    int cursor_max_y; // track max y over all columns on row
  };

  struct layout_state {
    layout_type type;
    // layout bounds (global)
    //   rect.min - layout position
    //   rect.max.x != rect.min.x -> fixed size
    //   rect.max.y != rect.min.y -> fixed height
    im_rect rect;
    im_vec2 cursor;
    union {
      layout_none none = {};
      layout_row_data row;
    };
  };

  im_stack<layout_state> layout_state_stack_ = im_stack<layout_state>(32);
  im_stack<im_vec2> cursor_state_stack_ = im_stack<im_vec2>(32);
  im_vec2 cursor_ = im_vec2(0, 0);

  struct {
    im_rect rect;
  } widget_item_;

public:
  im_layout() = default;

  // Reserve space for widget item
  auto add_widget_item(int height) noexcept -> im_rect {
    if (height <= 0) [[unlikely]] {
      assert(false && "im_layout::add_widget_item(...) negative or zero height");
      return im_rect();
    }
    auto& layout = layout_state_stack_.back();
    if (layout.type != layout_type::container && layout.type != layout_type::column) {
      assert(false && "im_layout::add_widget_item(...) unexpected layout type");
      return im_rect();
    }

    widget_item_.rect.min = cursor_;
    widget_item_.rect.max = im_vec2(layout.rect.max.x, cursor_.y + height - 1);
    cursor_.y += height;

    return widget_item_.rect;
  }

  [[nodiscard]] auto get_widget_item_rect() const noexcept -> im_rect const& {
    return widget_item_.rect;
  }

  void container_begin(im_rect const& rect) {
    auto& container_layout = layout_state_stack_.emplace_back();
    container_layout.type = layout_type::container;
    container_layout.rect = rect;
    container_layout.none = {};

    cursor_state_stack_.emplace_back(cursor_);
    cursor_ = rect.min;
  }

  void container_end() {
    auto& container_layout = layout_state_stack_.back();
    if (container_layout.type != layout_type::container) [[unlikely]] {
      assert(false && "im_layout::container_end(...) out of order");
      return;
    }

    layout_state_stack_.pop_back();
    cursor_ = cursor_state_stack_.back();
    cursor_state_stack_.pop_back();
  }

  void row_begin(std::size_t columns) {
    if (columns == 0) [[unlikely]] {
      assert(false && "im_layout::row_begin(...) invalid argument");
      return;
    }

    cursor_state_stack_.push_back(cursor_);

    auto& container_layout = layout_state_stack_.back();
    auto& row_layout = layout_state_stack_.emplace_back();

    row_layout.type = layout_type::row;
    row_layout.rect.min = cursor_;
    row_layout.rect.max = im_vec2(container_layout.rect.max.x, cursor_.y);
    row_layout.row = layout_row_data{.columns = int(columns), .index = 0, .cursor_max_y = cursor_.y};

    // at this point
    // row_layout.rect.max.x != row_layout.rect.min.x -> layout width set
    // row_layout.rect.max.y == row_layout.rect.min.y -> layout height unset (dynamic)
  }

  void row_push(float ratio_or_width) {
    ratio_or_width = std::max<float>(0.0f, ratio_or_width);

    // offset position x since previous column
    int offset_min_x = 0;
    if (auto& column_layout = layout_state_stack_.back(); column_layout.type == layout_type::column) {
      offset_min_x = column_layout.rect.max.x + 1;
      layout_state_stack_.pop_back();
    }

    auto& row_layout = layout_state_stack_.back();
    if (row_layout.type != layout_type::row) [[unlikely]] {
      assert(false && "im_layout::row_push(...) out of order");
      return;
    }
    if (row_layout.row.index == row_layout.row.columns) [[unlikely]] {
      assert(false && "im_layout::row_push(...) max columns reached");
      return;
    }

    // adjust to row layout in case of no previous columns
    offset_min_x = std::max(offset_min_x, row_layout.rect.min.x);
    // update cursor max y
    row_layout.row.cursor_max_y = std::max(row_layout.row.cursor_max_y, cursor_.y);

    auto const width =
        ratio_or_width > 1.0f ? int(ratio_or_width) : int(std::ceil(ratio_or_width * row_layout.rect.width()));
    auto const offset_max_x = std::min<int>(offset_min_x + width - 1, row_layout.rect.max.x);

    auto& column_layout = layout_state_stack_.emplace_back();
    column_layout.type = layout_type::column;
    column_layout.rect.min = im_vec2(offset_min_x, row_layout.rect.min.y);
    column_layout.rect.max = im_vec2(offset_max_x, row_layout.rect.min.y);
    column_layout.none = {};

    row_layout.row.index++;

    cursor_ = column_layout.rect.min;
  }

  void row_end() {
    if (auto& column_layout = layout_state_stack_.back(); column_layout.type == layout_type::column) {
      layout_state_stack_.pop_back();
    }

    auto& row_layout = layout_state_stack_.back();
    if (row_layout.type != layout_type::row) [[unlikely]] {
      assert(false && "im_layout::row_end(...) out of order");
      return;
    }

    auto const cursor_max_y = std::max(row_layout.row.cursor_max_y, cursor_.y);
    layout_state_stack_.pop_back();

    cursor_ = im_vec2(cursor_state_stack_.back().x, cursor_max_y);
    cursor_state_stack_.pop_back();
  }

  void reset(im_rect const& rect) {
    layout_state_stack_.clear();
    cursor_state_stack_.clear();

    auto& layout = layout_state_stack_.emplace_back();
    layout.type = layout_type::container;
    layout.rect = rect;
    layout.none = {};

    cursor_ = layout.rect.min;
  }
};

} // namespace xxx
