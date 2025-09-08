// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx_internal.h"

namespace xxx::v2 {

void label(std::string_view text) {
  auto rect = layout_space_prepare();
  if (rect.empty_area() || text.empty()) {
    return;
  }

  auto const glyphs = utf8_to_unicode(text);
  auto const glyphs_length = static_cast<int>(glyphs.size());

  rect.set_width(std::min<int>(glyphs_length, rect.get_width()));
  rect.set_height(1);

  // TODO
  auto const hovered = is_mouse_hovering_rect(rect);
  auto const clicked = is_mouse_pressed(im_mouse_button_id::left);
  auto const style = im_style(hovered ? (clicked ? 0x99ffee_c : 0xff9999_c) : 0xee6666_c);

  draw_text(rect.get_pos(), glyphs, style);

  layout_space_commit(rect.get_height());
}

} // namespace xxx::v2
