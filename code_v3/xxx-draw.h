// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <span>

#include "xxx-internal.h"

namespace xxx {

void draw_point(im_vec2 const& pos, std::uint32_t ch, im_style const& style) noexcept;
void draw_hline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept;
void draw_vline(im_vec2 const& pos, int length, std::uint32_t ch, im_style const& style) noexcept;
void draw_rect(im_rect const& rect, std::uint32_t ch, im_style const& style) noexcept;
void draw_border(im_rect const& rect, im_style const& style) noexcept;
void draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) noexcept;

} // namespace xxx
