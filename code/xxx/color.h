// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#ifndef KSERGEY_color_020919230636
#define KSERGEY_color_020919230636

#include <cstdint>
#include <type_traits>

namespace xxx {

/// Color type
enum class color : std::uint16_t { default_ = 0x00 };

/// Make color from RGB components
constexpr color make_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
  int r_ = (r * 5 + 0x7f) / 0xff;
  int g_ = (g * 5 + 0x7f) / 0xff;
  int b_ = (b * 5 + 0x7f) / 0xff;
  return static_cast<color>(0x10 + (r_ * 36 + g_ * 6 + b_));
}

namespace literals {

/// Make color from decimal number.
constexpr color operator""_c(unsigned long long int value) noexcept {
  int r = (((value >> 16) & 0xff) * 5 + 0x7f) / 0xff;
  int g = (((value >> 8) & 0xff) * 5 + 0x7f) / 0xff;
  int b = (((value >> 0) & 0xff) * 5 + 0x7f) / 0xff;
  return static_cast<color>(0x10 + (r * 36 + g * 6 + b));
}

static_assert(0x556270_c == make_color(85, 98, 112));
static_assert(0xFF6B6B_c == make_color(255, 107, 107));

}  // namespace literals

}  // namespace xxx

#endif /* KSERGEY_color_020919230636 */
