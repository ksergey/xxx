// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace xxx {

struct im_color {
  std::uint32_t value = 0;

  constexpr im_color() noexcept {}

  constexpr im_color(float r, float g, float b) noexcept
      : value((std::uint32_t(std::uint8_t(r * 255)) << 16) | std::uint32_t(std::uint8_t(g * 255)) << 8 |
              std::uint32_t(std::uint8_t(b * 255))) {}

  constexpr im_color(std::uint32_t v) noexcept : value(v) {}

  constexpr operator std::uint32_t() const noexcept {
    return value;
  }

  constexpr auto operator<=>(im_color const&) const noexcept = default;
};

inline namespace literals {

[[nodiscard]] constexpr auto operator""_c(unsigned long long int value) noexcept -> im_color {
  return im_color(value);
}

} // namespace literals
} // namespace xxx
