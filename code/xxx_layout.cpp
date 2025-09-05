// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx_layout.h"

#include <cassert>
#include <cmath>

namespace xxx::v2 {

void layout_reset() {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  layouts_stack.clear();

  auto& layout = layouts_stack.emplace_back();
  layout.type = im_layout_type::container;
  layout.bounds = im_rect(0, 0, ::tb_width(), tb_height());
  layout.min_height = 0;
  layout.filled_height = 0;
  layout.row.filled_width = 0;
  layout.row.index = 0;
  layout.row.columns = 0;
}

auto layout_space_prepare(int height) -> im_rect {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  auto& layout = layouts_stack.back();

  auto rect = layout.bounds.adjusted_top(-layout.filled_height);
  if (height > 0) {
    rect.height = std::min<int>(height, rect.height);
  }
  return rect;
}

void layout_set_min_height(int height) {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  auto& layout = layouts_stack.back();

  layout.min_height = height;
}

void layout_space_commit(int height) {
  assert(height >= 0);

  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  auto& layout = layouts_stack.back();

  layout.filled_height = std::min<int>(layout.filled_height + height, layout.bounds.height);
}

void layout_row_begin(int height, std::size_t columns) {
  if (columns == 0) [[unlikely]] {
    assert(false && "layout_row_xxx columns invalid argument");
    return;
  }

  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  auto& container_layout = layouts_stack.back();
  auto& row_layout = layouts_stack.emplace_back();

  row_layout.type = im_layout_type::row;
  row_layout.bounds = container_layout.bounds.adjusted_top(-container_layout.filled_height);
  if (height > 0) {
    row_layout.bounds.height = std::min<int>(height, row_layout.bounds.height);
  }
  row_layout.min_height = 0;
  row_layout.filled_height = 0;
  row_layout.row.filled_width = 0;
  row_layout.row.index = 0;
  row_layout.row.columns = columns;
}

void layout_row_push(float ratio_or_width) {
  ratio_or_width = std::max<float>(0.0f, ratio_or_width);

  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;

  int filled_height = 0;
  int filled_width = 0;

  if (auto& column_layout = layouts_stack.back(); column_layout.type == im_layout_type::column) {
    filled_height = std::max<int>(column_layout.filled_height, column_layout.min_height);
    filled_width = column_layout.bounds.width;
    layouts_stack.pop_back();
  }

  auto& row_layout = layouts_stack.back();
  if (row_layout.type != im_layout_type::row) [[unlikely]] {
    assert(false && "layout_row_xxx out of order");
    return;
  }
  if (row_layout.row.index == row_layout.row.columns) [[unlikely]] {
    assert(false && "layout_row_xxx max columns reached");
    return;
  }

  row_layout.filled_height = std::max<int>(filled_height, row_layout.filled_height);
  row_layout.row.filled_width += filled_width;
  assert(row_layout.row.filled_width <= row_layout.bounds.width);

  auto const width =
      ratio_or_width > 1.0f ? int(ratio_or_width) : int(std::lround(ratio_or_width * row_layout.bounds.width));

  auto& column_layout = layouts_stack.emplace_back();
  column_layout.type = im_layout_type::column;
  column_layout.bounds = row_layout.bounds.adjusted_left(-row_layout.row.filled_width);
  column_layout.bounds.width = std::min<int>(width, column_layout.bounds.width);
  column_layout.min_height = 0;
  column_layout.filled_height = 0;
  column_layout.row.filled_width = 0;
  column_layout.row.index = 0;
  column_layout.row.columns = 0;

  row_layout.row.index++;
}

void layout_row_end() {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;

  int filled_height = 0;

  if (auto& column_layout = layouts_stack.back(); column_layout.type == im_layout_type::column) {
    filled_height = std::max<int>(column_layout.filled_height, column_layout.min_height);
    layouts_stack.pop_back();
  }

  auto& row_layout = layouts_stack.back();
  if (row_layout.type != im_layout_type::row) [[unlikely]] {
    assert(false && "layout_row_xxx out of order");
    return;
  }

  filled_height = std::max<int>({row_layout.filled_height, row_layout.min_height, filled_height});
  layouts_stack.pop_back();

  auto& container_layout = layouts_stack.back();
  container_layout.filled_height += filled_height;
}

} // namespace xxx::v2
