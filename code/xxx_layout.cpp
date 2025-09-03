// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx_layout.h"

#include <cassert>

namespace xxx::v2 {

void clear_layout() {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  layouts_stack.resize(1);

  auto& layout = layouts_stack.front();
  layout.pos = {0, 0};
  layout.size = {::tb_width(), ::tb_height()};
  layout.filled = {0, 0};
  layout.columns = 0;
  layout.column = 0;
}

auto reserve_space(int height) -> im_rect {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  auto& layout = layouts_stack.back();

  height = std::min<int>(height, layout.size.y - layout.filled.y);

  auto min = im_vec2{layout.pos.x, layout.pos.y + layout.filled.y};
  auto max = im_vec2{min.x + layout.size.x, min.y + height};

  return im_rect(min, max);
}

void commit_space(int height) {
  auto const ctx = get_context();
  assert(ctx);

  auto& layouts_stack = ctx->layouts_stack;
  auto& layout = layouts_stack.back();

  layout.filled.y += height;
}

} // namespace xxx::v2
