// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

namespace xxx {

// add widget item
// --------------     --------------
// |            |     |            |
// |            |  -> |            |
// |            |     |            |
// *-------------     --------------
// pen                |   height   |
//                    *-------------
//                    (new pen pos)
void layout_add_widget_item(int height);

void layout_container_begin(im_rect const& rect);
void layout_container_end();

void layout_row_begin(std::size_t columns);
void layout_row_push(float ratio_or_width);
void layout_row_end();

} // namespace xxx
