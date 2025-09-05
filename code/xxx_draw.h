// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <ranges>

#include "xxx_internal.h"

namespace xxx::v2 {

struct basic_drawer {
  static inline void draw_point(im_vec2 const& pos, std::uint32_t ch, im_style const& style) noexcept {
    ::tb_set_cell(pos.x, pos.y, ch, style.fg, style.bg);
  }

  static inline void draw_hline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {
    for (int pos_x : std::views::iota(pos.x) | std::views::take(length)) {
      ::tb_set_cell(pos_x, pos.y, ch, style.fg, style.bg);
    }
  }

  static inline void draw_vline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {
    for (int pos_y : std::views::iota(pos.y) | std::views::take(length)) {
      ::tb_set_cell(pos.x, pos_y, ch, style.fg, style.bg);
    }
  }

  static inline void draw_text(
      im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) noexcept {
    for (auto const entry : std::views::zip(std::ranges::iota_view(pos.x), text)) {
      ::tb_set_cell(std::get<0>(entry), pos.y, std::get<1>(entry), style.fg, style.bg);
    }
  }
};

struct clip_drawer {
  im_rect const& clip;

  inline void draw_point(im_vec2 const& pos, std::uint32_t ch, im_style const& style) noexcept {
    if (clip.contains(pos)) {
      ::tb_set_cell(pos.x, pos.y, ch, style.fg, style.bg);
    }
  }

  inline void draw_hline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {
    // FIXME
    for (int pos_x : std::views::iota(pos.x) | std::views::take(length)) {
      ::tb_set_cell(pos_x, pos.y, ch, style.fg, style.bg);
    }
  }

  inline void draw_vline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {
    // FIXME
    for (int pos_y : std::views::iota(pos.y) | std::views::take(length)) {
      ::tb_set_cell(pos.x, pos_y, ch, style.fg, style.bg);
    }
  }

  inline void draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) noexcept {
    // FIXME
    for (auto const entry : std::views::zip(std::ranges::iota_view(pos.x), text)) {
      ::tb_set_cell(std::get<0>(entry), pos.y, std::get<1>(entry), style.fg, style.bg);
    }
  }
};

} // namespace xxx::v2
