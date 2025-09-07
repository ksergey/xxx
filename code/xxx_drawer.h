// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <span>

#include "xxx_internal.h"

namespace xxx {

class drawer {
private:
  im_vec2 translate_ = im_vec2{};
  im_rect clip_rect_ = im_rect{};

public:
  drawer(drawer const&) = delete;
  drawer& operator=(drawer const&) = delete;
  drawer() = default;

  void clip_rect(im_rect const& rect) noexcept {
    clip_rect_ = rect;
  }
  void translate(im_vec2 const& value) noexcept {
    translate_ = value;
  }

  void draw_point(im_vec2 const& pos, std::uint32_t ch, im_style const& style) noexcept {}

  void draw_hline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {}

  void draw_vline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept {}

  void draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) noexcept {}
};

} // namespace xxx
