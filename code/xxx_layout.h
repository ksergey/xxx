// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include "xxx_internal.h"

namespace xxx::v2 {

// reset
void layout_reset();

[[nodiscard]] auto layout_space_prepare(int height = 0) -> im_rect;
void layout_space_commit(int height);

// set layout minimal height
void layout_set_min_height(int height);

void layout_row_begin(int height, std::size_t columns);
void layout_row_push(float ratio_or_width);
void layout_row_end();

} // namespace xxx::v2
