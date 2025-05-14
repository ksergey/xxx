// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

#include <termbox2.h>

namespace xxx {

/// Attribute
enum class Attribute : uintattr_t {
  Bold = TB_BOLD,
  Underline = TB_UNDERLINE,
  Reverse = TB_REVERSE,
  Italic = TB_ITALIC,
  Bright = TB_BRIGHT,
  Dim = TB_DIM
};

/// Color
enum class Color : uintattr_t {};

/// Alignment
enum class Alignment { Left, Center, Right };

/// Make Color from RGB components
constexpr Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
  return Color((std::uint32_t(r) << 16) | std::uint32_t(g) << 8 | std::uint32_t(b));
}

namespace literals {

/// Make Color from decimal number
constexpr Color operator""_c(unsigned long long int value) noexcept {
  return Color(value);
}

} // namespace literals

namespace detail {

template <typename T, typename Tag = struct Default>
inline T& getStorageFor() noexcept {
  static T value = T();
  return value;
}

} // namespace detail

/// Init. Throws on error
void init();

/// Shutdown
void shutdown();

/// Process terminal events
/// @return true on an tty event received
bool update(unsigned ms = 33) noexcept;

/// Return true on key pressed
bool isKeyPressed(std::uint16_t key) noexcept;

/// Begin frame
void begin();

/// End frame
void end();

/// Push drawing style
void stylePush(Color fg, Attrribute attr = Attrribute::None, Color bg = Color());

/// Pop drawing style
void stylePop();

/// Push style color
void styleColorPush(ColorID idx, Color color);

/// Pop style color
void styleColorPop(std::size_t count = 1);

/// Start drawing row
/// @param[in] columns is number of columns inside row
void rowBegin(std::size_t columns);

/// Start next column in row
/// @param[in] ratioOrWidth is column percentage width (value <= 1.0) or explicit width
void rowPush(float ratioOrWidth);

/// Stop drawing row
void rowEnd();

/// Begin drawing panel
void panelBegin(std::string_view title = {});

/// End drawing panel
void panelEnd();

/// Draw single line text
void label(std::string_view text, Align align = Align::Left);

/// Add empty area
/// @param[in] ratioOrHeight is height percentage or explicit height
void spacer(float ratioOrHeight = 1.0);

/// Draw spinner
/// @param step is storage for spinner state
void spinner(std::string_view text = {}, Align align = Align::Left, float& step = detail::getStorageFor<float>());

/// Draw progress bar
void progress(float& value);

/// Draw text input. Return true on input finished
bool textInput(std::string& input);

/// Interface for drawing on canvas
class Canvas {
private:
  int const width_;
  int const height_;

public:
  Canvas(int width, int height) noexcept : width_(width), height_(height) {}

  virtual ~Canvas() noexcept {}

  int width() const noexcept {
    return width_;
  }

  int height() const noexcept {
    return height_;
  }

  virtual void point(int x, int y, Color color = Color::Default) = 0;
};

/// Draw anything in canvas
void canvas(float ratioOrHeight, std::function<void(Canvas&)> const& fn);

} // namespace xxx
