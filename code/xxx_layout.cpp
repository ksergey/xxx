// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx_layout.h"

#include <cassert>
#include <cmath>

namespace xxx::v2 {
namespace {

constexpr auto consume_top(im_rect const& rect, int height) noexcept -> im_rect {
  return im_rect(rect.x, rect.y + height, rect.width, rect.height - height);
}

constexpr auto consume_left(im_rect const& rect, int width) noexcept -> im_rect {
  return im_rect(rect.x + width, rect.y, rect.width - width, rect.height);
}

} // namespace

void layout_reset() {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  layouts_stack.clear();

  auto& layout = layouts_stack.emplace_back();
  layout.type = im_layout_type::container;
  layout.bounds = im_rect(0, 0, ::tb_width(), tb_height());
  layout.filled_height = 0;
  layout.row.filled_width = 0;
  layout.row.index = 0;
  layout.row.columns = 0;
}

auto layout_space_reserve() -> im_rect {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  auto& layout = layouts_stack.back();

  return consume_top(layout.bounds, layout.filled_height);
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
  row_layout.bounds = consume_top(container_layout.bounds, container_layout.filled_height);
  if (height > 0) {
    row_layout.bounds.height = std::min<int>(row_layout.bounds.height, height);
  }
  row_layout.filled_height = 0;
  row_layout.row.filled_width = 0;
  row_layout.row.index = 0;
  row_layout.row.columns = columns;
}

void layout_row_push(float ratio_or_width) {
  ratio_or_width = std::max<float>(ratio_or_width, 0.0f);

  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;

  int filled_height = 0;
  int filled_width = 0;

  if (auto& column_layout = layouts_stack.back(); column_layout.type == im_layout_type::column) {
    filled_height = column_layout.filled_height;
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

  row_layout.filled_height = std::max<int>(row_layout.filled_height, filled_height);
  row_layout.row.filled_width += filled_width;
  assert(row_layout.row.filled_width <= row_layout.bounds.width);

  auto const width =
      ratio_or_width > 1.0f ? int(ratio_or_width) : int(std::lround(ratio_or_width * row_layout.bounds.width));

  auto& column_layout = layouts_stack.emplace_back();
  column_layout.type = im_layout_type::column;
  column_layout.bounds = consume_left(row_layout.bounds, row_layout.row.filled_width);
  column_layout.bounds.width = std::min<int>(width, column_layout.bounds.width);
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
    filled_height = column_layout.filled_height;
    layouts_stack.pop_back();
  }

  auto& row_layout = layouts_stack.back();
  if (row_layout.type != im_layout_type::row) [[unlikely]] {
    assert(false && "layout_row_xxx out of order");
    return;
  }

  filled_height = std::max<int>(row_layout.filled_height, filled_height);
  layouts_stack.pop_back();

  auto& container_layout = layouts_stack.back();
  container_layout.filled_height += filled_height;
}

} // namespace xxx::v2
