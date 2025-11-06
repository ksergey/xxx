// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <format>

#include "im_vec2.h"

namespace xxx {

/// helper: aligned bounding-box
struct im_rect {
  im_vec2 min = im_vec2(0, 0);   // top-left (included)
  im_vec2 max = im_vec2(-1, -1); // bottom-right (included)

  constexpr im_rect() = default;

  constexpr im_rect(im_vec2 const min0, im_vec2 const& max0) noexcept : min(min0), max(max0) {}

  constexpr im_rect(int x0, int y0, int x1, int y1) noexcept : min{x0, y0}, max{x1, y1} {}

  [[nodiscard]] constexpr operator bool() const noexcept {
    return !this->empty();
  }
  [[nodiscard]] constexpr auto null() const noexcept -> bool {
    return (min.x - 1 == max.x) && (min.y - 1 == max.y);
  }
  [[nodiscard]] constexpr auto empty() const noexcept -> bool {
    return (min.x > max.x) || (min.y > max.y);
  }
  [[nodiscard]] constexpr auto valid() const noexcept -> bool {
    return (min.x <= max.x) && (min.y <= max.y);
  }
  [[nodiscard]] constexpr auto center() const noexcept -> im_vec2 {
    return im_vec2((min.x + max.x) / 2, (min.y + max.y) / 2);
  }
  [[nodiscard]] constexpr auto width() const noexcept -> int {
    return max.x - min.x + 1;
  }
  constexpr void set_width(int v) noexcept {
    max.x = min.x + v - 1;
  }
  [[nodiscard]] constexpr auto height() const noexcept -> int {
    return max.y - min.y + 1;
  }
  constexpr void set_height(int v) noexcept {
    max.y = min.y + v - 1;
  }
  [[nodiscard]] constexpr auto size() const noexcept -> im_vec2 {
    return im_vec2(this->width(), this->height());
  }
  constexpr void set_size(im_vec2 const& v) noexcept {
    this->set_width(v.x);
    this->set_height(v.y);
  }
  [[nodiscard]] constexpr auto top_left() const noexcept -> im_vec2 {
    return min;
  }
  [[nodiscard]] constexpr auto bottom_right() const noexcept -> im_vec2 {
    return max;
  }
  [[nodiscard]] constexpr auto top_right() const noexcept -> im_vec2 {
    return im_vec2(max.x, min.y);
  }
  [[nodiscard]] constexpr auto bottom_left() const noexcept -> im_vec2 {
    return im_vec2(min.x, max.y);
  }
  [[nodiscard]] constexpr auto contains(im_vec2 const& p) const noexcept -> bool {
    return p.x >= min.x && p.y >= min.y && p.x <= max.x && p.y <= max.y;
  }
  [[nodiscard]] constexpr auto contains(im_rect const& r) const noexcept -> bool {
    return r.min.x >= min.x && r.min.y >= min.y && r.max.x <= max.x && r.max.y <= max.y;
  }
  [[nodiscard]] constexpr auto intersection(im_rect const& r) const noexcept -> im_rect {
    if (this->null() || r.null()) {
      return im_rect();
    }
    auto const& r_min = r.min;
    auto const& r_max = r.max;
    // TODO
    if (r_max.x < min.x || r_min.x > max.x || r_max.y < min.y || r_min.y > max.y) {
      return im_rect();
    }
    return im_rect(im_vec2(std::clamp(min.x, r_min.x, r_max.x), std::clamp(min.y, r_min.y, r_max.y)),
        im_vec2(std::clamp(max.x, r_min.x, r_max.x), std::clamp(max.y, r_min.y, r_max.y)));
  }
  [[nodiscard]] constexpr auto translate(im_vec2 const& t) const noexcept -> im_rect {
    return im_rect(min + t, max + t);
  }
  [[nodiscard]] constexpr auto crop(int left, int top, int right, int bottom) const noexcept -> im_rect {
    return im_rect(min.x + left, min.y + top, max.x - right, max.y - bottom);
  }
  [[nodiscard]] constexpr auto crop(int value) const noexcept -> im_rect {
    return this->crop(value, value, value, value);
  }
  [[nodiscard]] constexpr auto crop_left(int value) const noexcept -> im_rect {
    return this->crop(value, 0, 0, 0);
  }
  [[nodiscard]] constexpr auto crop_top(int value) const noexcept -> im_rect {
    return this->crop(0, value, 0, 0);
  }
  [[nodiscard]] constexpr auto crop_right(int value) const noexcept -> im_rect {
    return this->crop(0, 0, value, 0);
  }
  [[nodiscard]] constexpr auto crop_bottom(int value) const noexcept -> im_rect {
    return this->crop(0, 0, 0, value);
  }

  constexpr auto operator<=>(im_rect const&) const noexcept = default;
};

} // namespace xxx

template <>
struct std::formatter<xxx::im_rect> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }
  auto format(xxx::im_rect const& r, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "({}, {})", r.min, r.max);
  }
};
