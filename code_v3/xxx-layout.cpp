// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx-internal.h"

#include <algorithm>
#include <cassert>
#include <cmath>

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
  ::fclose(file);
}

} // namespace xxx
#endif

namespace xxx {

void layout_add_widget_item(int height) {
  auto const ctx = get_context();
  auto& layout = ctx->layout_stack.back();
  auto& widget = ctx->widget;

  widget.bounds.min = ctx->cursor;
  widget.bounds.max = im_vec2(layout.bounds.max.x, ctx->cursor.y + height - 1);

  ctx->cursor.y += height;
}

void layout_container_begin(im_rect const& rect) {
  auto const ctx = get_context();

  auto& layout_stack = ctx->layout_stack;
  auto& container_layout = layout_stack.emplace_back();

  container_layout.type = im_layout_type::container;
  container_layout.bounds = rect;
  container_layout.none = {};

  ctx->cursor = rect.min;
  ctx->renderer.push_clip_rect(rect);
}

void layout_container_end() {
  auto const ctx = get_context();

  auto& layout_stack = ctx->layout_stack;

  auto& container_layout = layout_stack.back();
  if (container_layout.type != im_layout_type::container) [[unlikely]] {
    assert(false && "layout_container_xxx out of order");
    return;
  }

  ctx->renderer.pop_clip_rect();

  layout_stack.pop_back();

  auto& parent_layout = layout_stack.back();
  ctx->cursor = parent_layout.bounds.min;
}

void layout_row_begin(std::size_t columns) {
  if (columns == 0) [[unlikely]] {
    assert(false && "layout_row_xxx columns invalid argument");
    return;
  }

  auto const ctx = get_context();

  ctx->cursor_stack.push_back(ctx->cursor);

  auto& layout_stack = ctx->layout_stack;
  auto& container_layout = layout_stack.back();
  auto& row_layout = layout_stack.emplace_back();

  row_layout.type = im_layout_type::row;
  row_layout.bounds.min = ctx->cursor;
  row_layout.bounds.max = im_vec2(container_layout.bounds.max.x, ctx->cursor.y);

  row_layout.row = im_layout_row{
      .columns = int(columns),
      .index = 0,
  };
}

void layout_row_push(float ratio_or_width) {
  ratio_or_width = std::max<float>(0.0f, ratio_or_width);

  auto const ctx = get_context();

  auto& layout_stack = ctx->layout_stack;

  int offset_x = 0;
  if (auto& column_layout = layout_stack.back(); column_layout.type == im_layout_type::column) {
    offset_x = column_layout.bounds.max.x + 1;
    layout_stack.pop_back();
  }

  auto& row_layout = layout_stack.back();
  if (row_layout.type != im_layout_type::row) [[unlikely]] {
    assert(false && "layout_row_xxx out of order");
    return;
  }
  if (row_layout.row.index == row_layout.row.columns) [[unlikely]] {
    assert(false && "layout_row_xxx max columns reached");
    return;
  }

  offset_x = std::max(offset_x, row_layout.bounds.min.x);

  row_layout.bounds.max.y = std::max(row_layout.bounds.max.y, ctx->cursor.y);

  auto const width =
      ratio_or_width > 1.0f ? int(ratio_or_width) : int(std::lround(ratio_or_width * row_layout.bounds.width()));

  auto& column_layout = layout_stack.emplace_back();
  column_layout.type = im_layout_type::column;
  column_layout.bounds.min = im_vec2(offset_x, row_layout.bounds.min.y);
  column_layout.bounds.max = im_vec2(offset_x + width - 1, row_layout.bounds.min.y);
  column_layout.none = im_layout_none{};

  row_layout.row.index++;

  ctx->cursor = column_layout.bounds.min;
}

void layout_row_end() {
  auto const ctx = get_context();

  auto& layout_stack = ctx->layout_stack;

  if (auto& column_layout = layout_stack.back(); column_layout.type == im_layout_type::column) {
    layout_stack.pop_back();
  }

  auto& row_layout = layout_stack.back();
  if (row_layout.type != im_layout_type::row) [[unlikely]] {
    assert(false && "layout_row_xxx out of order");
    return;
  }

  auto const max_cursor_y = std::max(row_layout.bounds.max.y, ctx->cursor.y);
  layout_stack.pop_back();

  auto& container_layout = layout_stack.back();
  container_layout.bounds.max.y = max_cursor_y;

  ctx->cursor = im_vec2(ctx->cursor_stack.back().x, max_cursor_y);
  ctx->cursor_stack.pop_back();
}

} // namespace xxx
