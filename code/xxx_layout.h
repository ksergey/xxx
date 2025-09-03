// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include "xxx_internal.h"

namespace xxx::v2 {

void clear_layout();
auto reserve_space(int height) -> im_rect;
void commit_space(int height);

} // namespace xxx::v2
