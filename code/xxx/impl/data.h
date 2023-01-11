// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#pragma once

#include <array>
#include <chrono>
#include <string>
#include <type_traits>
#include <vector>

#include <xxx/ui.h>

namespace xxx::impl {

struct Size {
  int width = 0;
  int height = 0;
};

struct Point {
  int x = 0;
  int y = 0;
};

namespace style {

/// Border style.
struct Border {
  std::uint32_t verticalLine;
  std::uint32_t horizontalLine;
  std::uint32_t upperLeftCorner;
  std::uint32_t upperRightCorner;
  std::uint32_t bottomLeftCorner;
  std::uint32_t bottomRightCorner;
};

} // namespace style

enum class LayoutType { Container, Row, Column };

struct LayoutState {
  LayoutType type = LayoutType::Container;
  Size size;
  Point pos;
  Size filledSize;
  std::size_t columns = 0;
  std::size_t column = 0;
  bool fillBackground = false;
};

static_assert(std::is_trivially_copyable_v<LayoutState>);

/// Clock for internal usage.
using Clock = std::chrono::steady_clock;

/// UI global context.
struct Context {
  struct {
    /// Panel widget style.
    struct {
      Color titleColor;
      Color borderColor;
      style::Border border;
    } panel;

    /// Spinner widget style.
    struct {
      Color spinnerColor;
      Color labelColor;
      std::vector<std::uint32_t> glyphs;
    } spinner;

    /// Progress widget style.
    struct {
      Color barColor;
      Color labelColor;
      std::uint32_t barGlyph;
    } progress;

    /// Text input widget.
    struct {
      Color fg;
      Color bg;
    } textInput;
  } style;

  /// Layout stack.
  std::vector<LayoutState> layoutStack;

  /// Text input chars.
  std::u32string inputQueueChars;

  /// Pressed keys.
  std::array<bool, 512> pressedKeys;

  /// Screen size (updates once per cycle).
  Size screenSize;

  /// Last clock timestamp.
  Clock::time_point timestamp;

  /// Time step.
  float deltaTime = 0.0;
};

} // namespace xxx::impl
