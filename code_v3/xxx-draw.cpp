// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "xxx-draw.h"

#include <array>
#include <cassert>
#include <ranges>

namespace xxx {
namespace {

inline auto get_clip_rect() noexcept -> im_rect {
  auto const ctx = get_context();
  assert(ctx);
  return ctx->layouts.front().bounds;
}

} // namespace

void draw_point(im_vec2 const& pos, std::uint32_t ch, im_style const& style) noexcept {
  auto const clip_rect = get_clip_rect();

  if (clip_rect.contains(pos)) {
    ::tb_set_cell(pos.x, pos.y, ch, style.fg, style.bg);
  }
}

void draw_hline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {
  draw_rect(im_rect(pos, pos + im_vec2(length, 1)), ch, style);
}

void draw_vline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {
  draw_rect(im_rect(pos, pos + im_vec2(1, length)), ch, style);
}

void draw_rect(im_rect const& rect, std::uint32_t ch, im_style const& style) noexcept {
  auto const clip_rect = get_clip_rect();

  auto const c_rect = rect.intersection(clip_rect);
  for (int pos_x : std::views::iota(c_rect.min.x, c_rect.max.x + 1)) {
    for (int pos_y : std::views::iota(c_rect.min.y, c_rect.max.y + 1)) {
      ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
    }
  }
}

void draw_border(im_rect const& rect, im_style const& style) noexcept {
  constexpr auto borders = std::to_array<std::uint32_t>({L'╭', L'╮', L'╰', L'╯', L'│', L'─'});

  auto const clip_rect = get_clip_rect();
  if (clip_rect.empty() || rect.empty()) {
    return;
  }

  auto const top_left = rect.top_left();
  auto const top_right = rect.top_right();
  auto const bottom_left = rect.bottom_left();
  auto const bottom_right = rect.bottom_right();

  auto const hline_min_x = std::clamp<int>(top_left.x + 1, clip_rect.min.x, clip_rect.max.x);
  auto const hline_max_x = std::clamp<int>(top_right.x - 1, clip_rect.min.x, clip_rect.max.x);

  auto const vline_min_y = std::clamp<int>(top_left.y + 1, clip_rect.min.y, clip_rect.max.y);
  auto const vline_max_y = std::clamp<int>(bottom_left.y - 1, clip_rect.min.y, clip_rect.max.y);

  if (top_left.y >= clip_rect.min.y) {
    if (top_left.x >= clip_rect.min.x) {
      ::tb_set_cell(top_left.x, top_left.y, borders[0], style.fg, style.bg);
    }
    if (top_right.x <= clip_rect.max.x) {
      ::tb_set_cell(top_right.x, top_right.y, borders[1], style.fg, style.bg);
    }
    if (hline_min_x < hline_max_x) {
      for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::iota(hline_min_x, hline_max_x + 1),
               std::views::repeat(rect.min.y), std::views::repeat(borders[5]))) {
        ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
      }
    }
  }

  if (bottom_left.y <= clip_rect.max.y) {
    if (bottom_left.x >= clip_rect.min.x) {
      ::tb_set_cell(bottom_left.x, bottom_left.y, borders[2], style.fg, style.bg);
    }
    if (bottom_right.x <= clip_rect.max.x) {
      ::tb_set_cell(bottom_right.x, bottom_right.y, borders[3], style.fg, style.bg);
    }
    if (hline_min_x < hline_max_x) {
      for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::iota(hline_min_x, hline_max_x + 1),
               std::views::repeat(rect.max.y), std::views::repeat(borders[5]))) {
        ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
      }
    }
  }

  if (vline_min_y < vline_max_y) {
    if (top_left.x >= clip_rect.min.x) {
      for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::repeat(rect.min.x),
               std::views::iota(vline_min_y, vline_max_y + 1), std::views::repeat(borders[4]))) {
        ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
      }
    }
    if (top_right.x <= clip_rect.max.x) {
      for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::repeat(rect.max.x),
               std::views::iota(vline_min_y, vline_max_y + 1), std::views::repeat(borders[4]))) {
        ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
      }
    }
  }
}

void draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) noexcept {
  auto const clip_rect = get_clip_rect();

  auto const rect = im_rect(pos, pos + im_vec2(text.size(), 1));
  auto const c_rect = rect.intersection(clip_rect);
  text = text.subspan(static_cast<std::size_t>(c_rect.min.x - rect.min.x));

  for (auto const& [pos_x, pos_y, ch] :
      std::views::zip(std::views::iota(c_rect.min.x, c_rect.max.x), std::views::repeat(c_rect.min.y), text)) {
    ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
  }
}

} // namespace xxx
