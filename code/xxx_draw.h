// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include "xxx_internal.h"

namespace xxx::v2 {

inline void draw_point(int x, int y, std::uint32_t ch, im_style const& style) noexcept {
  ::tb_set_cell(x, y, ch, style.fg, style.bg);
}

inline void draw_hline(int x, int y, int length, std::uint32_t ch, im_style const& style) noexcept {
  for (int cur_x = x, end_x = cur_x + length; cur_x < end_x; ++cur_x) {
    draw_point(cur_x, y, ch, style);
  }
}

inline void draw_vline(int x, int y, int length, std::uint32_t ch, im_style const& style) noexcept {
  for (int pos_y = y, end_y = pos_y + length; pos_y < end_y; ++pos_y) {
    draw_point(x, pos_y, ch, style);
  }
}

inline void draw_text(int x, int y, std::span<std::uint32_t const> text, im_style const& style) noexcept {
  for (auto const ch : text) {
    draw_point(x++, y, ch, style);
  }
}

inline void draw_point(im_vec2 const& pos, std::uint32_t ch, im_style const& style) noexcept {
  return draw_point(pos.x, pos.y, ch, style);
}

inline void draw_hline(im_vec2 pos, int length, std::uint32_t ch, im_style const& style) noexcept {
  return draw_hline(pos.x, pos.y, length, ch, style);
}

inline void draw_vline(im_vec2 pos, int length, std::uint32_t ch, im_style const& style) noexcept {
  return draw_vline(pos.x, pos.y, length, ch, style);
}

inline void draw_text(im_vec2 pos, std::span<std::uint32_t const> text, im_style const& style) noexcept {
  return draw_text(pos.x, pos.y, std::move(text), style);
}

} // namespace xxx::v2
