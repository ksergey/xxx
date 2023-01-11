// ------------------------------------------------------------
// Copyright 2022-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#pragma once

#include <cstdint>

namespace xxx {

/// Attributes.
enum class Attribute : std::uint16_t { Bold = 0x0100, Underline = 0x200, Reverse = 0x0400 };

/// Color type.
enum class Color : std::uint16_t { Default = 0x00 };

/// Add attribute to color.
constexpr Color operator|(Color lhs, Attribute rhs) noexcept {
  return static_cast<Color>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

/// Make Color from RGB components.
constexpr Color makeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
  int r_ = (r * 5 + 0x7f) / 0xff;
  int g_ = (g * 5 + 0x7f) / 0xff;
  int b_ = (b * 5 + 0x7f) / 0xff;
  return static_cast<Color>(0x10 + (r_ * 36 + g_ * 6 + b_));
}

namespace literals {

/// Make Color from decimal number.
constexpr Color operator""_c(unsigned long long int value) noexcept {
  int r = (((value >> 16) & 0xff) * 5 + 0x7f) / 0xff;
  int g = (((value >> 8) & 0xff) * 5 + 0x7f) / 0xff;
  int b = (((value >> 0) & 0xff) * 5 + 0x7f) / 0xff;
  return static_cast<Color>(0x10 + (r * 36 + g * 6 + b));
}

} // namespace literals

enum class Key : std::uint16_t {
  Esc = 0x1B,
  Enter = 0x0D,
  F1 = (0xFFFF - 0),
  F2 = (0xFFFF - 1),
  F3 = (0xFFFF - 2),
  F4 = (0xFFFF - 3),
  F5 = (0xFFFF - 4),
  F6 = (0xFFFF - 5),
  F7 = (0xFFFF - 6),
  F8 = (0xFFFF - 7),
  F9 = (0xFFFF - 8),
  F10 = (0xFFFF - 9),
  F11 = (0xFFFF - 10),
  F12 = (0xFFFF - 11),
  Insert = (0xFFFF - 12),
  Delete = (0xFFFF - 13),
  Home = (0xFFFF - 14),
  End = (0xFFFF - 15),
  PgUp = (0xFFFF - 16),
  PgDown = (0xFFFF - 17),
  ArrowUp = (0xFFFF - 18),
  ArrowDown = (0xFFFF - 19),
  ArrowLeft = (0xFFFF - 20),
  ArrowRight = (0xFFFF - 21)
};

/// Align.
enum class Align { Left, Center, Right };

} // namespace xxx
