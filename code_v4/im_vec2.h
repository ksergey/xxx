// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace xxx {

// helper: 2d vector
struct im_vec2 {
  int x = 0;
  int y = 0;

  constexpr im_vec2() = default;
  constexpr im_vec2(int x0, int y0) noexcept : x(x0), y(y0) {}

  [[nodiscard]] friend constexpr auto operator+(im_vec2 const& a, im_vec2 const& b) noexcept -> im_vec2 {
    return im_vec2(a.x + b.x, a.y + b.y);
  }
  [[nodiscard]] friend constexpr auto operator-(im_vec2 const& a, im_vec2 const& b) noexcept -> im_vec2 {
    return im_vec2(a.x - b.x, a.y - b.y);
  }
  [[nodiscard]] constexpr auto operator-() const noexcept -> im_vec2 {
    return im_vec2(-x, -y);
  }
  constexpr auto operator+=(im_vec2 const& a) noexcept -> im_vec2& {
    x += a.x;
    y += a.y;
    return *this;
  }
  constexpr auto operator-=(im_vec2 const& a) noexcept -> im_vec2& {
    x -= a.x;
    y -= a.y;
    return *this;
  }

  constexpr auto operator<=>(im_vec2 const&) const noexcept = default;
};

} // namespace xxx
