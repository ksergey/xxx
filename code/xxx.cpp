// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "xxx.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <optional>
#include <span>
#include <vector>

#ifndef XXX_FORCE_INLINE
#define XXX_FORCE_INLINE inline __attribute__((always_inline))
#endif

namespace xxx {

struct Style {
  uintattr_t fg = 0;
  uintattr_t bg = 0;
};

struct Point {
  int x = 0;
  int y = 0;
};

struct Size {
  int width = 0;
  int height = 0;
};

struct Rect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

enum class LayoutType { Container, Row, Column };

struct Layout {
  LayoutType type = LayoutType::Container;
  Size size;
  Point pos;
  Size filledSize;
  unsigned columnsCount = 0;
  unsigned column = 0;
};

static_assert(std::is_trivially_copyable_v<Layout>);

struct BorderStyle {
  std::uint32_t vLine;
  std::uint32_t hLine;
  std::uint32_t topLeft;
  std::uint32_t topRight;
  std::uint32_t bottomLeft;
  std::uint32_t bottomRight;
};

struct SpinnerStyle {
  std::vector<std::uint32_t> glyphs;
};

struct ProgressBarStyle {
  std::uint32_t glyph;
};

struct Context {
  // Panel border style
  BorderStyle borderStyle = BorderStyle{L'│', L'─', L'╭', L'╮', L'╰', L'╯'};

  // Spinner style
  SpinnerStyle spinnerStyle = SpinnerStyle{{L'⠉', L'⠑', L'⠃', L'⠊', L'⠒', L'⠢', L'⠆', L'⠔', L'⠤', L'⢄', L'⡄', L'⡠',
      L'⣀', L'⢄', L'⢠', L'⡠', L'⠤', L'⠢', L'⠰', L'⠔', L'⠒', L'⠑', L'⠘', L'⠊'}};

  // Progress bar style
  ProgressBarStyle progressBarStyle = ProgressBarStyle{L'■'};

  // Stack of styles
  std::vector<Style> styleStack;

  // Stack of layouts
  std::vector<Layout> layoutStack;

  // Spinner spin interval
  float const spinnerUpdateInterval = 0.1; // seconds

  // last key event (tb_event.type = TB_EVENT_KEY)
  std::optional<InputEvent> lastInputEvent;

  // previous clockNow() value
  std::uint64_t previousNow = 0;

  // elapsed seconds since last update
  float elapsed = 0.0;
};

namespace {

[[nodiscard]] XXX_FORCE_INLINE Context* currentContext() noexcept {
  static Context ctx;
  return &ctx;
}

[[nodiscard]] XXX_FORCE_INLINE std::uint64_t clockNowMs() noexcept {
  ::timespec ts;
  ::clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Result available until next call
[[nodiscard]] std::span<std::uint32_t const> UTF8CharToUnicode(std::string_view str) noexcept {
  // Unicode string cache
  thread_local std::vector<std::uint32_t> cache;

  cache.resize(str.size());

  char const* begin = str.data();
  char const* end = begin + str.size();
  std::size_t pos = 0;

  while (begin < end) {
    if (*begin == '\0') {
      break;
    }
    auto const length = ::tb_utf8_char_length(*begin);
    if (begin + length > end) [[unlikely]] {
      break;
    }
    ::tb_utf8_char_to_unicode(&cache[pos++], begin);
    begin += length;
  }

  return std::span(cache.data(), pos);
}

[[nodiscard]] XXX_FORCE_INLINE constexpr int alignSize(int innerSize, int parentSize, Alignment align) noexcept {
  if (parentSize > innerSize) [[likely]] {
    switch (align) {
    case Alignment::Center:
      return (parentSize - innerSize) / 2;
    case Alignment::Right:
      return (parentSize - innerSize);
    default:
      break;
    }
  }
  return 0;
}

} // namespace

namespace draw {

XXX_FORCE_INLINE void point(int x, int y, std::uint32_t ch, Style const& style) noexcept {
  ::tb_set_cell(x, y, ch, style.fg, style.bg);
}

XXX_FORCE_INLINE void hLine(int x, int y, int length, std::uint32_t ch, Style const& style = {}) noexcept {
  for (int pos = x, end = pos + length; pos < end; ++pos) {
    point(pos, y, ch, style);
  }
}

XXX_FORCE_INLINE void vLine(int x, int y, int length, std::uint32_t ch, Style const& style = {}) noexcept {
  for (int pos = y, end = pos + length; pos < end; ++pos) {
    point(x, pos, ch, style);
  }
}

XXX_FORCE_INLINE void text(int x, int y, std::span<std::uint32_t const> text, Style const& style) noexcept {
  for (auto const ch : text) {
    point(x++, y, ch, style);
  }
}

XXX_FORCE_INLINE void text(int x, int y, std::string_view text, Style const& style) noexcept {
  for (auto const ch : text) {
    point(x++, y, static_cast<std::uint32_t>(ch), style);
  }
}

} // namespace draw

XXX_FORCE_INLINE Rect reserveSpace(int height) noexcept {
  auto const ctx = currentContext();

  height = std::max<int>(height, 0);

  auto& layout = ctx->layoutStack.back();

  Rect result;
  result.x = layout.pos.x;
  result.y = layout.pos.y + layout.filledSize.height;
  result.width = layout.size.width;
  result.height = std::min<int>(height, layout.size.height - layout.filledSize.height);

  layout.filledSize.height += result.height;

  return result;
}

#if 0
std::string inputEventToKey(InputEvent const& ev)
{
    if (ev.ch == 0) {
        return std::string();
    }

    std::string key;

    if ((ev.mod & TB_MOD_CTRL) == TB_MOD_CTRL) {
        key.append("C-");
    }
    if ((ev.mod & TB_MOD_ALT) == TB_MOD_ALT) {
        key.append("M-");
    }
    if ((ev.mod & TB_MOD_SHIFT) == TB_MOD_SHIFT) {
        key.append("S-");
    }

    if (ev.ch <= 0x10ffff && !(ev.ch >= 0xd800 && ev.ch <= 0xdfff)) {
        char value[6];
        ::tb_utf8_unicode_to_char(value, ev.ch);
        key.append(value);
    }

    return key;
}
#endif

void init() {
  auto const ctx = currentContext();

  if (auto const rc = ::tb_init(); rc != TB_OK) {
    throw std::runtime_error(::tb_strerror(rc));
  }
  ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);

  ctx->styleStack.reserve(32);
  ctx->layoutStack.reserve(32);
  ctx->previousNow = clockNowMs();
}

void shutdown() {
  ::tb_shutdown();
}

bool update(unsigned timeoutMs) {
  auto const ctx = currentContext();

  ::tb_event event;
  auto const rc = ::tb_peek_event(&event, static_cast<int>(timeoutMs));
  if (rc != TB_OK && rc != TB_ERR_NO_EVENT) [[unlikely]] {
    if (!(rc == TB_ERR_POLL && tb_last_errno() == EINTR)) {
      throw std::runtime_error(::tb_strerror(rc));
    }
  }

  // cleanup before update
  ctx->lastInputEvent = std::nullopt;

  // process event
  if (rc == TB_OK) {
    switch (event.type) {
    case TB_EVENT_KEY: {
      ctx->lastInputEvent = InputEvent{event.mod, event.key, event.ch};
    } break;
    case TB_EVENT_MOUSE: {
    } break;
    case TB_EVENT_RESIZE: {
    } break;
    default: {
    } break;
    }
  }

  auto const now = clockNowMs();
  ctx->elapsed += (now - ctx->previousNow) * 0.001f;
  ctx->previousNow = now;

  return rc == TB_OK;
}

InputEvent const* lastInputEvent() {
  auto const ctx = currentContext();

  return ctx->lastInputEvent ? &(*ctx->lastInputEvent) : nullptr;
}

void stylePush(Color fg, Color bg, Attribute attr) {
  auto const ctx = currentContext();

  auto& style = ctx->styleStack.emplace_back();
  style.fg = static_cast<uintattr_t>(fg) | static_cast<uintattr_t>(attr);
  style.bg = static_cast<uintattr_t>(bg);
}

void stylePop() {
  auto const ctx = currentContext();

  if (ctx->styleStack.size() == 1) [[unlikely]] {
    assert(false && "Calling stylePop() too many times!");
    return;
  }
  ctx->styleStack.pop_back();
}

void begin() {
  auto const ctx = currentContext();

  // init style
  ctx->styleStack.clear();
  ctx->styleStack.emplace_back();

  // init layout
  ctx->layoutStack.clear();
  auto& layout = ctx->layoutStack.emplace_back();
  layout.type = LayoutType::Container;
  layout.size = Size{::tb_width(), ::tb_height()};
  layout.pos = {0, 0};
  layout.filledSize = {0, 0};

  ::tb_clear();
}

void end() {
  auto const ctx = currentContext();

  ::tb_present();

  ctx->elapsed = 0.0f;
}

void rowBegin(unsigned columnsCount) {
  auto const ctx = currentContext();

  if (columnsCount == 0) [[unlikely]] {
    return;
  }

  auto& parent = ctx->layoutStack.back();

  Layout layout;
  layout.type = LayoutType::Row;
  layout.size = {parent.size.width, parent.size.height - parent.filledSize.height};
  layout.pos = {parent.pos.x, parent.pos.y + parent.filledSize.height};
  layout.columnsCount = columnsCount;
  layout.column = 0;
  layout.filledSize = {0, 0};

  ctx->layoutStack.emplace_back(std::move(layout));
}

void rowEnd() {
  auto const ctx = currentContext();

  int rowFilledHeight = 0;

  if (auto& layout = ctx->layoutStack.back(); layout.type == LayoutType::Column) {
    rowFilledHeight = layout.filledSize.height;
    ctx->layoutStack.pop_back();
  }

  if (auto& layout = ctx->layoutStack.back(); layout.type != LayoutType::Row) [[unlikely]] {
    // Unexpected layout state
    assert(false && "out of order call");
    return;
  }

  rowFilledHeight = std::max<int>(rowFilledHeight, ctx->layoutStack.back().filledSize.height);
  ctx->layoutStack.pop_back();

  // Update parent layout filled height.
  ctx->layoutStack.back().filledSize.height += rowFilledHeight;
}

void rowPush(float widthOrRatio) {
  auto const ctx = currentContext();

  widthOrRatio = std::max<float>(widthOrRatio, 0.0f);

  if (auto& parent = ctx->layoutStack.back(); parent.type == LayoutType::Row) {
    assert(parent.filledSize.width == 0);
    assert(parent.column == 0);

    Layout layout;
    layout.type = LayoutType::Column;
    layout.pos = parent.pos;

    auto const width = widthOrRatio > 1.0f ? std::min<int>(widthOrRatio, parent.size.width)
                                           : int(std::round(widthOrRatio * parent.size.width));

    layout.size = {width, parent.size.height};

    parent.filledSize.width = layout.size.width;
    parent.column += 1;

    ctx->layoutStack.emplace_back(std::move(layout));

    return;
  }

  if (auto& parent = ctx->layoutStack.back(); parent.type != LayoutType::Column) {
    return;
  }

  auto const filledHeight = ctx->layoutStack.back().filledSize.height;
  ctx->layoutStack.pop_back();

  auto& parent = ctx->layoutStack.back();
  if (parent.type != LayoutType::Row) [[unlikely]] {
    assert(false && "out of order call");
    return;
  }
  if (parent.column >= parent.columnsCount) [[unlikely]] {
    assert(false && "out of order call");
    return;
  }

  parent.filledSize.height = std::max<int>(parent.filledSize.height, filledHeight);

  Layout layout;
  layout.type = LayoutType::Column;
  layout.pos = {parent.pos.x + parent.filledSize.width, parent.pos.y};
  auto const availableWidth = parent.size.width - parent.filledSize.width;
  auto const width = widthOrRatio > 1.0f ? std::min<int>(widthOrRatio, availableWidth)
                                         : std::min<int>(std::round(widthOrRatio * parent.size.width), availableWidth);
  layout.size = {width, parent.size.height};

  parent.filledSize.width += layout.size.width;
  parent.column += 1;

  ctx->layoutStack.emplace_back(std::move(layout));
}

void panelBegin() {
  auto const ctx = currentContext();

  auto& parent = ctx->layoutStack.back();
  if (parent.size.width < 2 || parent.size.height < 2) [[unlikely]] {
    return;
  }

  Layout layout;
  layout.type = LayoutType::Container;
  layout.size = {parent.size.width - 2, parent.size.height - parent.filledSize.height - 2};
  layout.pos = {parent.pos.x + 1, parent.pos.y + parent.filledSize.height + 1};

  auto const& style = ctx->styleStack.back();
  auto const& borderStyle = ctx->borderStyle;

  draw::point(layout.pos.x - 1, layout.pos.y - 1, borderStyle.topLeft, style);
  draw::hLine(layout.pos.x, layout.pos.y - 1, layout.size.width + 1, borderStyle.hLine, style);
  draw::point(layout.pos.x - 1 + layout.size.width + 1, layout.pos.y - 1, borderStyle.topRight, style);

  ctx->layoutStack.emplace_back(std::move(layout));
}

void panelEnd() {
  auto const ctx = currentContext();

  if (auto const& layout = ctx->layoutStack.back(); layout.type != LayoutType::Container) [[unlikely]] {
    assert(false && "out of order call");
    return;
  }

  auto const filledHeight = ctx->layoutStack.back().filledSize.height;
  ctx->layoutStack.pop_back();

  auto& parent = ctx->layoutStack.back();
  parent.filledSize.height += filledHeight + 2;

  auto const& style = ctx->styleStack.back();
  auto const& borderStyle = ctx->borderStyle;

  draw::point(parent.pos.x, parent.pos.y + parent.filledSize.height - 1, borderStyle.bottomLeft, style);
  draw::hLine(
      parent.pos.x + 1, parent.pos.y + parent.filledSize.height - 1, parent.size.width - 2, borderStyle.hLine, style);
  draw::point(parent.pos.x + parent.size.width - 1, parent.pos.y + parent.filledSize.height - 1,
      borderStyle.bottomRight, style);
  draw::vLine(parent.pos.x, parent.pos.y + parent.filledSize.height - (filledHeight + 1), filledHeight,
      borderStyle.vLine, style);
  draw::vLine(parent.pos.x + parent.size.width - 1, parent.pos.y + parent.filledSize.height - (filledHeight + 1),
      filledHeight, borderStyle.vLine, style);
}

void panelTitle(std::string_view text, Alignment align) {
  auto const ctx = currentContext();

  if (auto const& layout = ctx->layoutStack.back(); layout.type != LayoutType::Container) [[unlikely]] {
    assert(false && "out of order call");
    return;
  }

  auto& parent = ctx->layoutStack.back();

  auto const unicodeStr = UTF8CharToUnicode(text);
  if (parent.size.width < 2 || unicodeStr.empty()) [[unlikely]] {
    // nothing to draw or no space
    return;
  }

  auto const textSize = std::min<int>(unicodeStr.size(), parent.size.width - 2);
  auto const textOffsetX = alignSize(textSize, parent.size.width - 2, align);

  assert(!ctx->styleStack.empty() && "styles stack empty");

  auto const& style = ctx->styleStack.back();

  draw::text(parent.pos.x + textOffsetX + 1, parent.pos.y - 1, unicodeStr.subspan(0, textSize), style);
}

void label(std::string_view text, Alignment align) {
  auto const ctx = currentContext();

  auto const unicodeStr = UTF8CharToUnicode(text);
  auto const rect = reserveSpace(1);

  if (rect.width < 1 || rect.height < 1 || unicodeStr.empty()) [[unlikely]] {
    // nothing to draw or no space
    return;
  }

  auto const textSize = std::min<int>(unicodeStr.size(), rect.width);
  auto const textOffsetX = alignSize(textSize, rect.width, align);

  assert(!ctx->styleStack.empty() && "styles stack empty");

  auto const& style = ctx->styleStack.back();

  draw::text(rect.x + textOffsetX, rect.y, unicodeStr.subspan(0, textSize), style);
}

void spacer(float heightOrRatio) {
  auto const ctx = currentContext();

  heightOrRatio = std::max<float>(heightOrRatio, 0.0f);

  auto& parent = ctx->layoutStack.back();
  auto const availableHeight = parent.size.height - parent.filledSize.height;

  auto const height = heightOrRatio > 1.0f
                          ? std::min<int>(heightOrRatio, availableHeight)
                          : std::min<int>(std::round(heightOrRatio * availableHeight), availableHeight);
  reserveSpace(height);
}

namespace impl {

void spinner(std::string_view text, Alignment align, float& step) {
  auto const ctx = currentContext();

  auto const unicodeStr = UTF8CharToUnicode(text);
  auto const rect = reserveSpace(1);

  auto const length = static_cast<int>(unicodeStr.size() + 1);

  if (rect.width < 1 || rect.height < 1) [[unlikely]] {
    // nothing to draw or no space
    return;
  }

  auto const textSize = std::min<int>(length, rect.width);
  auto const textOffsetX = alignSize(textSize, rect.width, align);

  step += ctx->elapsed;
  auto const index = std::size_t(std::round(step / ctx->spinnerUpdateInterval)) % ctx->spinnerStyle.glyphs.size();

  assert(!ctx->spinnerStyle.glyphs.empty() && "spinner style glyphs empty");
  assert(!ctx->styleStack.empty() && "styles stack empty");

  auto const& style = ctx->styleStack.back();
  auto const& spinnerStyle = ctx->spinnerStyle;

  draw::point(rect.x + textOffsetX, rect.y, spinnerStyle.glyphs[index], style);
  draw::text(rect.x + textOffsetX + 1, rect.y, unicodeStr.subspan(0, textSize - 1), style);
}

} // namespace impl

void progress(float& value) {
  auto const ctx = currentContext();

  auto const rect = reserveSpace(1);
  if (rect.width < 1 || rect.height < 1) [[unlikely]] {
    return;
  }

  value = std::clamp<float>(value, 0.0, 100.0);
  auto const length = static_cast<int>(std::round((rect.width * value) / 100.0));

  assert(!ctx->styleStack.empty() && "styles stack empty");

  char buffer[sizeof(" 100.0%  ")];
  std::snprintf(buffer, sizeof(buffer), " %02.1f%% ", static_cast<double>(value));
  auto const text = std::string_view(buffer);

  auto const textSize = std::min<int>(text.size(), rect.width);
  auto const textOffsetX = alignSize(textSize, rect.width, Alignment::Center);

  auto const& style = ctx->styleStack.back();
  auto const& progressBarStyle = ctx->progressBarStyle;

  draw::hLine(rect.x, rect.y, length, progressBarStyle.glyph, style);
  draw::hLine(rect.x + length, rect.y, rect.width - length, L' ', style);
  draw::text(rect.x + textOffsetX, rect.y, std::string_view(buffer), style);
}

bool textInput(std::string& input, bool active, Alignment align) {
  auto const ctx = currentContext();

  bool isEnterPressed = false;

  if (active && ctx->lastInputEvent) {
    auto const& lastInput = *ctx->lastInputEvent;
    if (lastInput.key == TB_KEY_CTRL_W) {
      while (!input.empty() && std::isblank(input.back())) {
        input.pop_back();
      }
      auto const found = input.rfind(' ');
      if (found == input.npos) {
        input.clear();
      } else {
        input.erase(input.begin() + found + 1, input.end());
      }
    } else if (lastInput.key == TB_KEY_ENTER) {
      isEnterPressed = true;
    }
    // TODO: backspace, space, etc...
    if (lastInput.ch > 0) {
      input += lastInput.ch;
    }
  }

  label(input, align);

  return isEnterPressed;
}

namespace {

constexpr std::array kBraillePixelMap = {
    std::array{0x01, 0x08}, std::array{0x02, 0x10}, std::array{0x04, 0x20}, std::array{0x40, 0x80}};
constexpr std::uint32_t kBrailleOffset = 0x2800;

} // namespace

Canvas::Canvas(int startX, int startY, int width, int height) noexcept
    : startX_(startX), startY_(startY), width_(width), height_(height) {}

void Canvas::point(int x, int y, Color color) {
  if (x < 0 || x >= width() || y < 0 || y >= height()) [[unlikely]] {
    return;
  }

  auto const posX = startX_ + x / 2;
  auto const posY = startY_ + y / 4;
  auto& cell = ::tb_cell_buffer()[posY * ::tb_width() + posX];

  if (cell.ch < kBrailleOffset) {
    cell.ch = kBrailleOffset;
  }
  cell.ch |= kBraillePixelMap[y % 4][x % 2];
  cell.fg = static_cast<uintattr_t>(color);
}

Canvas canvas(float heightOrRatio) {
  auto const ctx = currentContext();

  heightOrRatio = std::max<float>(heightOrRatio, 0.0f);

  auto& parent = ctx->layoutStack.back();
  auto const availableHeight = parent.size.height - parent.filledSize.height;

  auto const height = heightOrRatio > 1.0f
                          ? std::min<int>(heightOrRatio, availableHeight)
                          : std::min<int>(std::round(heightOrRatio * availableHeight), availableHeight);
  auto const rect = reserveSpace(height);

  return Canvas(rect.x, rect.y, rect.width * 2, rect.height * 4);
}

} // namespace xxx
