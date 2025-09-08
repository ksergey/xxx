// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx_internal.h"

#include <ranges>

namespace xxx::v2 {

void draw_fill_rect(im_rect const& rect, std::uint32_t ch, im_style const& style) noexcept {
  auto const& clip_rect = clip_rect_get();
  auto const& translate = translate_get();

  auto const tc_rect = rect.translated(translate).intersection(clip_rect);
  for (int tc_pos_x : std::views::iota(tc_rect.min.x, tc_rect.max.x)) {
    for (int tc_pos_y : std::views::iota(tc_rect.min.y, tc_rect.max.y)) {
      ::tb_set_cell(tc_pos_x, tc_pos_y, ch, style.fg, style.bg);
    }
  }
}

void draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) noexcept {
  auto const& clip_rect = clip_rect_get();
  auto const& translate = translate_get();

  auto const t_rect = im_rect(pos, pos + im_vec2(text.size(), 1)).translated(translate);
  auto const tc_rect = t_rect.intersection(clip_rect);
  text = text.subspan(static_cast<std::size_t>(tc_rect.min.x - t_rect.min.x));

  for (auto const entry :
      std::views::zip(std::ranges::iota_view(tc_rect.min.x), text) | std::views::take(tc_rect.get_width())) {
    auto const tc_pos_x = std::get<0>(entry);
    auto const ch = std::get<1>(entry);
    ::tb_set_cell(tc_pos_x, tc_rect.min.y, ch, style.fg, style.bg);
  }
}

} // namespace xxx::v2
