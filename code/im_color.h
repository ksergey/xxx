// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace xxx {

/// Color: terminal default, palette index (0-15 follow the user's terminal scheme, 16-255 xterm palette)
/// or 24-bit RGB. Encoded in 32 bits: 0x00RRGGBB is RGB (so 0x000000 is black), the high byte tags
/// the other kinds.
struct im_color {
  static constexpr std::uint32_t default_tag = 0x01000000u;
  static constexpr std::uint32_t indexed_tag = 0x02000000u;

  std::uint32_t value = default_tag;

  /// Terminal default color
  constexpr im_color() noexcept = default;

  /// RGB, e.g. im_color(0xdcf763)
  constexpr im_color(std::uint32_t v) noexcept : value(v) {}

  /// RGB from 0..1 components
  constexpr im_color(float r, float g, float b) noexcept
      : value((std::uint32_t(std::uint8_t(r * 255)) << 16) | std::uint32_t(std::uint8_t(g * 255)) << 8 |
              std::uint32_t(std::uint8_t(b * 255))) {}

  [[nodiscard]] static constexpr auto terminal_default() noexcept -> im_color {
    return im_color();
  }
  /// Palette color: 0-7 ANSI, 8-15 bright ANSI, 16-231 color cube, 232-255 grays
  [[nodiscard]] static constexpr auto indexed(std::uint8_t index) noexcept -> im_color {
    return im_color(indexed_tag | index);
  }
  [[nodiscard]] static constexpr auto rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept -> im_color {
    return im_color((std::uint32_t(r) << 16) | (std::uint32_t(g) << 8) | b);
  }

  [[nodiscard]] constexpr auto is_default() const noexcept -> bool {
    return value == default_tag;
  }
  [[nodiscard]] constexpr auto is_indexed() const noexcept -> bool {
    return (value & 0xff000000u) == indexed_tag;
  }
  [[nodiscard]] constexpr auto is_rgb() const noexcept -> bool {
    return (value & 0xff000000u) == 0;
  }
  [[nodiscard]] constexpr auto index() const noexcept -> std::uint8_t {
    return std::uint8_t(value & 0xff);
  }

  constexpr operator std::uint32_t() const noexcept {
    return value;
  }
  constexpr auto operator<=>(im_color const&) const noexcept = default;
};

/// The 16 colors of the user's terminal scheme: adapt to light and dark backgrounds
namespace ansi {
inline constexpr auto black = im_color::indexed(0);
inline constexpr auto red = im_color::indexed(1);
inline constexpr auto green = im_color::indexed(2);
inline constexpr auto yellow = im_color::indexed(3);
inline constexpr auto blue = im_color::indexed(4);
inline constexpr auto magenta = im_color::indexed(5);
inline constexpr auto cyan = im_color::indexed(6);
inline constexpr auto white = im_color::indexed(7);
inline constexpr auto bright_black = im_color::indexed(8);
inline constexpr auto bright_red = im_color::indexed(9);
inline constexpr auto bright_green = im_color::indexed(10);
inline constexpr auto bright_yellow = im_color::indexed(11);
inline constexpr auto bright_blue = im_color::indexed(12);
inline constexpr auto bright_magenta = im_color::indexed(13);
inline constexpr auto bright_cyan = im_color::indexed(14);
inline constexpr auto bright_white = im_color::indexed(15);
} // namespace ansi

inline namespace literals {

/// RGB literal: 0xdcf763_c
[[nodiscard]] constexpr auto operator""_c(unsigned long long int value) noexcept -> im_color {
  return im_color(std::uint32_t(value & 0xffffff));
}

} // namespace literals

} // namespace xxx
