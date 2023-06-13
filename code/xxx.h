// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace xxx {

/// Attributes
enum class Attr : std::uint16_t { Bold = 0x0100, Underline = 0x200, Reverse = 0x0400 };

/// Color
enum class Color : std::uint16_t { Default = 0x00 };

/// Align
enum class Align { Left, Center, Right };

/// Add attribute to color
constexpr Color operator|(Color lhs, Attr rhs) noexcept {
  return static_cast<Color>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

/// Make Color from RGB components
constexpr Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
  int r_ = (r * 5 + 0x7f) / 0xff;
  int g_ = (g * 5 + 0x7f) / 0xff;
  int b_ = (b * 5 + 0x7f) / 0xff;
  return static_cast<Color>(0x10 + (r_ * 36 + g_ * 6 + b_));
}

namespace literals {

/// Make Color from decimal number
constexpr Color operator""_c(unsigned long long int value) noexcept {
  int r = (((value >> 16) & 0xff) * 5 + 0x7f) / 0xff;
  int g = (((value >> 8) & 0xff) * 5 + 0x7f) / 0xff;
  int b = (((value >> 0) & 0xff) * 5 + 0x7f) / 0xff;
  return static_cast<Color>(0x10 + (r * 36 + g * 6 + b));
}

} // namespace literals

namespace key {

static constexpr auto Esc = std::uint16_t(0x1b);
static constexpr auto Enter = std::uint16_t(0x0d);
static constexpr auto ArrowUp = std::uint16_t(0xFFFF - 18);
static constexpr auto ArrowDown = std::uint16_t(0xFFFF - 19);
static constexpr auto ArrowLeft = std::uint16_t(0xFFFF - 20);
static constexpr auto ArrowRight = std::uint16_t(0xFFFF - 21);

} // namespace key

namespace detail {

template<typename T, typename Tag = struct Default>
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

/// Draw single line text with custom color
void label(std::string_view text, Color color, Align align = Align::Left);

/// Draw single line text
void label(std::string_view text, Align align = Align::Left);

/// Draw single line warning text
void warning(std::string_view text, Align align = Align::Left);

/// Draw single line error text
void error(std::string_view text, Align align = Align::Left);

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
