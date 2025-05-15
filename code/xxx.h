// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>

#include <termbox2.h>

namespace xxx {
namespace impl {

template <typename T, typename Tag>
[[nodiscard]] T& getStorageFor() noexcept {
  static T value = T();
  return value;
}

} // namespace impl

/// Color
enum class Color : uintattr_t {};

/// Construct \c Color from RGB components
constexpr Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
  return Color((std::uint32_t(r) << 16) | std::uint32_t(g) << 8 | std::uint32_t(b));
}

/// Text attributes
enum class Attribute : uintattr_t {
  None = 0,
  Bold = TB_BOLD,
  Underline = TB_UNDERLINE,
  Reverse = TB_REVERSE,
  Italic = TB_ITALIC,
  Bright = TB_BRIGHT,
  Dim = TB_DIM
};

/// Text alignment
enum class Alignment { Left, Right, Center };

/// Keyboard input event
struct InputEvent {
  std::uint8_t mod;  // bitwise `TB_MOD_*` constants
  std::uint16_t key; // one of `TB_KEY_*` constants
  std::uint32_t ch;  // a Unicode codepoint
};

#if 0
std::string inputEventToKey(InputEvent const& ev);
#endif

/// Init internals
void init();

/// Shutdown internals
void shutdown();

/// Read mouse/keyboard/etc events
/// \param[in] timeoutMs is number of milliseconds to wait event
/// \return true on an event read
bool update(unsigned timeoutMs = 100);

/// Return last input event
[[nodiscard]] InputEvent const* lastInputEvent();

/// Push draw style
void stylePush(Color fg, Color bg = Color(), Attribute attr = Attribute::None);

/// Pop draw style
void stylePop();

/// Start draw frame
void begin();

/// Finish draw frame
void end();

void rowBegin(unsigned columnsCount);
void rowEnd();
void rowPush(float widthOrRatio);

void panelBegin();
void panelEnd();
void panelTitle(std::string_view text, Alignment align = Alignment::Center);

/// Draw single line label
void label(std::string_view text, Alignment align = Alignment::Left);

/// Draw spacer
/// \param[in] heightOrRatio - spacer size if heightOrRatio > 1 or spacer ratio if heightOrRatio <= 1.0
void spacer(float heightOrRatio);

namespace impl {

void spinner(std::string_view text, Alignment align, float& step);

} // namespace impl

/// Draw spinner
template <typename Tag = struct Tag_DefaultSpinner>
void spinner(std::string_view text = {}, Alignment align = Alignment::Center) {
  impl::spinner(text, align, impl::getStorageFor<float, Tag>());
}

/// Draw progress bar
void progress(float& value);

/// Text input
bool textInput(std::string& input, bool active, Alignment align = Alignment::Left);

/// Canvas for drawing
class Canvas {
private:
  int startX_ = -1;
  int startY_ = -1;
  int width_ = -1;
  int height_ = -1;

public:
  Canvas(int startX, int startY, int width, int height) noexcept;

  /// Canvas width
  [[nodiscard]] int width() const noexcept {
    return width_;
  }

  /// Canvas height
  [[nodiscard]] int height() const noexcept {
    return height_;
  }

  /// Draw point
  void point(int x, int y, Color color = {});
};

/// Create cavas for drawing
[[nodiscard]] Canvas canvas(float heightOrRatio);

} // namespace xxx
