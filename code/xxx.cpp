// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "xxx.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <ctime>
#include <stdexcept>
#include <vector>

namespace xxx {

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

struct LayoutState {
  LayoutType type = LayoutType::Container;
  Size size;
  Point pos;
  Size filledSize;
  std::size_t columns = 0;
  std::size_t column = 0;
};

static_assert(std::is_trivially_copyable_v<LayoutState>);

struct BorderStyle {
  std::uint32_t vLine;
  std::uint32_t hLine;
  std::uint32_t upperLeft;
  std::uint32_t upperRight;
  std::uint32_t bottomLeft;
  std::uint32_t bottomRight;
};

struct PanelStyle {
  BorderStyle border;
};

struct SpinnerStyle {
  std::vector<std::uint32_t> glyphs;
};

struct ProgressStyle {
  std::uint32_t barGlyph;
};

struct ColorState {
  ColorID idx;
  Color color;
};

struct Style {
  PanelStyle panel;
  SpinnerStyle spinner;
  ProgressStyle progress;
};

namespace {

constexpr auto kColorsCount = static_cast<std::size_t>(ColorID::LAST);

struct {
  // widget styles
  Style style;

  // colors
  Color colors[kColorsCount];

  // layout stack
  std::vector<LayoutState> layoutStack;

  // colors stack
  std::vector<ColorState> colorsStack;

  // input characters during update
  std::vector<int> inputChars;

  // pressed keys
  std::array<bool, 512> pressedKeys;

  // terminal size
  Size screenSize;

  std::uint64_t timestamp = 0;

  float timeDelta = 0.0;

} ctx;

} // namespace

namespace utf8 {
namespace {

// clang-format off
static constexpr unsigned char lengthTable[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};
// clang-format on

static constexpr unsigned char maskTable[6] = {0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

} // namespace

constexpr std::size_t charLength(char ch) noexcept {
  return lengthTable[static_cast<unsigned char>(ch)];
}

constexpr bool isUtf8Trail(char ch) noexcept {
  return (static_cast<unsigned char>(ch) >> 6) == 0x2;
}

constexpr std::size_t stringLength(std::string_view str) noexcept {
  std::size_t result = 0;
  for (std::size_t i = 0; i < str.size();) {
    i += charLength(str[i]);
    ++result;
  }
  return result;
}

template<class OctectIterator>
constexpr std::uint32_t readUCS(OctectIterator it) noexcept {
  auto const length = charLength(*it);
  auto const mask = maskTable[length - 1];
  std::uint32_t result = *it & mask;
  for (std::size_t i = 1; i < length; ++i) {
    ++it;
    result = (result << 6) | (*it & 0x3f);
  }
  return result;
}

template<class OctectIterator>
[[nodiscard]] inline OctectIterator next(OctectIterator it) noexcept {
  if (*it != '\0') [[likely]] {
    std::advance(it, charLength(*it));
  }
  return it;
}

template<class OctectIterator>
[[nodiscard]] inline OctectIterator prev(OctectIterator it) noexcept {
  while (isUtf8Trail(*(--it))) {
  }
  return it;
}

template<class OctectIterator>
class Iterator {
private:
  OctectIterator it_{};

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = std::uint32_t;
  using pointer = value_type*;
  using reference = value_type&;

  Iterator() = default;

  explicit Iterator(OctectIterator it) : it_(std::move(it)) {}

  OctectIterator base() const {
    return it_;
  }

  value_type operator*() const noexcept {
    return readUCS(it_);
  }

  friend bool operator==(Iterator const& lhs, Iterator const& rhs) noexcept {
    return lhs.it_ == rhs.it_;
  }

  friend bool operator!=(Iterator const& lhs, Iterator const& rhs) noexcept {
    return lhs.it_ != rhs.it_;
  }

  Iterator& operator++() {
    std::advance(it_, charLength(*it_));
    return *this;
  }

  Iterator operator++(int) {
    Iterator temp(it_);
    std::advance(it_, charLength(*it_));
    return temp;
  }

  Iterator& operator--() {
    it_ = prev(it_);
    return *this;
  }

  Iterator operator--(int) {
    Iterator temp(*this);
    it_ = prev(it_);
    return temp;
  }
};

template<class OctectIterator>
inline auto makeIterator(OctectIterator it) {
  return Iterator<OctectIterator>(it);
}

template<class OctetIterator>
inline OctetIterator append(std::uint32_t ch, OctetIterator result) {
  if (ch < 0x80)
    *(result++) = static_cast<char>(ch);
  else if (ch < 0x800) {
    *(result++) = static_cast<char>((ch >> 6) | 0xc0);
    *(result++) = static_cast<char>((ch & 0x3f) | 0x80);
  } else if (ch < 0x10000) [[likely]] {
    *(result++) = static_cast<char>((ch >> 12) | 0xe0);
    *(result++) = static_cast<char>(((ch >> 6) & 0x3f) | 0x80);
    *(result++) = static_cast<char>((ch & 0x3f) | 0x80);
  } else {
    *(result++) = static_cast<char>((ch >> 18) | 0xf0);
    *(result++) = static_cast<char>(((ch >> 12) & 0x3f) | 0x80);
    *(result++) = static_cast<char>(((ch >> 6) & 0x3f) | 0x80);
    *(result++) = static_cast<char>((ch & 0x3f) | 0x80);
  }
  return result;
}

/// Convert sequence of uint32_t into utf8 string.
template<class OctectIterator, class U32Iterator>
inline OctectIterator convert(U32Iterator begin, U32Iterator end, OctectIterator result) {
  while (begin != end) {
    result = append(*(begin++), result);
  }
  return result;
}

} // namespace utf8

namespace draw {

inline ::tb_cell cell(std::uint32_t ch, Color fg, Color bg) noexcept {
  return ::tb_cell{ch, static_cast<std::uint16_t>(fg), static_cast<std::uint16_t>(bg)};
}

inline void point(int x, int y, ::tb_cell const& cell) noexcept {
  ::tb_set_cell(x, y, cell.ch, cell.fg, cell.bg);
}

inline void hLine(int x, int y, int length, ::tb_cell const& cell) noexcept {
  while (length-- > 0) {
    point(x, y, cell);
    x += 1;
  }
}

inline void vLine(int x, int y, int length, ::tb_cell const& cell) noexcept {
  while (length-- > 0) {
    point(x, y, cell);
    y += 1;
  }
}

inline void fill(int x, int y, int width, int height, ::tb_cell const& cell) noexcept {
  while (height-- > 0) {
    hLine(x, y, width, cell);
    y += 1;
  }
}

inline void fill(Rect const& rect, ::tb_cell const& cell) noexcept {
  return fill(rect.x, rect.y, rect.width, rect.height, cell);
}

inline void text(int x, int y, char const* str, int length, int offset, Color fg, Color bg) {
  if (offset < 0) {
    offset = 0;
  }

  auto cell = draw::cell(' ', fg, bg);
  auto it = utf8::makeIterator(str);

  if (offset > 0) {
    while (length > 0 && offset > 0) {
      --length;
      --offset;
      ++it;
    }
  }

  while (length > 0) {
    cell.ch = *it++;
    point(x++, y, cell);
    length -= 1;
  }
}

inline void text(int x, int y, char const* str, int length, Color fg, Color bg) {
  return text(x, y, str, length, 0, fg, bg);
}

} // namespace draw

// Get current color
inline Color& colorAt(ColorID idx) noexcept {
  auto const index = static_cast<std::size_t>(idx);
  assert(index < kColorsCount);
  return ctx.colors[index];
}

namespace {

void initStyle() {
  ctx.style.panel.border = {L'│', L'─', L'╭', L'╮', L'╰', L'╯'};

  ctx.style.spinner.glyphs = {{L'⠉', L'⠑', L'⠃', L'⠊', L'⠒', L'⠢', L'⠆', L'⠔', L'⠤', L'⢄', L'⡄', L'⡠',
                               L'⣀', L'⢄', L'⢠', L'⡠', L'⠤', L'⠢', L'⠰', L'⠔', L'⠒', L'⠑', L'⠘', L'⠊'}};

  ctx.style.progress.barGlyph = L'│';
}

void initColors() {
  using namespace literals;

  colorAt(ColorID::Text) = Color::Default;
  colorAt(ColorID::Background) = Color::Default;
  colorAt(ColorID::Border) = 0x666666_c;
  colorAt(ColorID::Glyph) = 0x66cc33_c | Attr::Bold;
}

} // namespace

inline std::uint64_t clockNow() noexcept {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ul + ts.tv_nsec;
}

inline std::size_t keyIdx(std::uint16_t key) noexcept {
  return key <= 0xFF ? key : (((0xFFFF - key) & 0xFF) + 256);
}

inline int alignWidth(int innerWidth, int parentWidth, Align align) noexcept {
  if (parentWidth > innerWidth) [[likely]] {
    switch (align) {
    case Align::Center:
      return (parentWidth - innerWidth) / 2;
    case Align::Right:
      return (parentWidth - innerWidth);
    default:
      break;
    }
  }
  return 0;
}

// Set current layout.
inline void pushLayout(LayoutState const& layout) {
  ctx.layoutStack.emplace_back(layout);
}

// Restore previous layout.
inline void popLayout() noexcept {
  ctx.layoutStack.pop_back();
}

// Reserve space inside layout.
inline Rect reserveSpace(int height) noexcept {
  if (height < 0) {
    height = 0;
  }

  auto& layout = ctx.layoutStack.back();

  Rect result;
  result.x = layout.pos.x;
  result.y = layout.pos.y + layout.filledSize.height;
  result.width = layout.size.width;
  result.height = std::min(height, layout.size.height - layout.filledSize.height);

  layout.filledSize.height += result.height;

  return result;
}

void init() {
  if (int const rc = ::tb_init(); rc != TB_OK) {
    throw std::runtime_error(::tb_strerror(rc));
  }
  ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);

  ctx.layoutStack.reserve(32);
  ctx.colorsStack.reserve(32);
  ctx.inputChars.reserve(32);

  initStyle();
  initColors();

  // Update clock.
  ctx.timestamp = clockNow();
}

void shutdown() {
  ::tb_shutdown();
}

bool update(unsigned ms) noexcept {
  ctx.pressedKeys.fill(false);

  ::tb_event event;

  std::size_t eventsProcessed = 0;

  auto const startTime = clockNow();
  while (true) {
    auto const now = clockNow();
    auto const dt = (now - startTime) / 1000000;
    if (dt >= ms) {
      break;
    }

    int const result = ::tb_peek_event(&event, ms);
    switch (result) {
    case TB_EVENT_KEY: {
      if (event.ch > 0) {
        ctx.inputChars.push_back(event.ch);
      }
      if (event.key > 0) {
        ctx.pressedKeys[keyIdx(event.key)] = true;
      }
    } break;
    case TB_EVENT_RESIZE:
    case TB_EVENT_MOUSE:
      break;
    default: {
    } break;
    }
    eventsProcessed += 1;
  }

  auto const now = clockNow();
  ctx.timeDelta = static_cast<double>(now - ctx.timestamp) / 1000000000.0;
  ctx.timestamp = now;

  return eventsProcessed > 0;
}

bool isKeyPressed(std::uint16_t key) noexcept {
  return ctx.pressedKeys[keyIdx(key)];
}

void begin() {
  ::tb_clear();

  ctx.screenSize = {::tb_width(), ::tb_height()};
  ctx.layoutStack.clear();

  auto& layout = ctx.layoutStack.emplace_back();
  layout.type = LayoutType::Container;
  layout.size = ctx.screenSize;
  layout.pos = {0, 0};
  layout.filledSize = {0, 0};

  styleColorPop(ctx.colorsStack.size());
}

void end() {
  ::tb_present();

  ctx.inputChars.clear();
}

void styleColorPush(ColorID idx, Color color) {
  auto& state = ctx.colorsStack.emplace_back();
  state.idx = idx;
  state.color = colorAt(idx);
  colorAt(idx) = color;
}

void styleColorPop(std::size_t count) {
  if (ctx.colorsStack.size() < count) [[unlikely]] {
    assert(false && "calling styleColorPop(...) too many times");
    count = ctx.colorsStack.size();
  }

  while (count-- > 0) {
    auto const& state = ctx.colorsStack.back();
    colorAt(state.idx) = state.color;
    ctx.colorsStack.pop_back();
  }
}

void rowBegin(std::size_t columns) {
  if (columns == 0) {
    return;
  }

  // Parent layout for new row layout.
  auto& parent = ctx.layoutStack.back();

  // New layout.
  LayoutState row;
  row.type = LayoutType::Row;
  row.size = {parent.size.width, parent.size.height - parent.filledSize.height};
  row.pos = {parent.pos.x, parent.pos.y + parent.filledSize.height};
  row.columns = columns;
  row.column = 0;
  row.filledSize = {0, 0};

  // Add layout to stack.
  pushLayout(row);
}

void rowPush(float ratioOrWidth) {
  auto const layout = []() -> LayoutState& {
    return ctx.layoutStack.back();
  };

  if (ratioOrWidth < 0.0) [[unlikely]] {
    ratioOrWidth = 0.0;
  }

  if (layout().type == LayoutType::Row) {
    auto& row = layout();

    assert(row.filledSize.width == 0);
    assert(row.column == 0);

    LayoutState column;
    column.type = LayoutType::Column;
    column.pos = row.pos;

    // Calculate column width.
    int width = 0;
    if (ratioOrWidth > 1.0) {
      width = std::min<int>(ratioOrWidth, row.size.width);
    } else {
      width = std::round(ratioOrWidth * row.size.width);
    }

    column.size = {width, row.size.height};

    row.filledSize.width = column.size.width;
    row.column += 1;

    // Add layout to stack.
    pushLayout(column);
  } else if (layout().type == LayoutType::Column) {
    int const filledHeight = layout().filledSize.height;
    // Remove prev column layout.
    popLayout();

    // Parent row layout.
    auto& row = layout();

    if (row.type != LayoutType::Row) [[unlikely]] {
      assert(false && "unexpected layout state");
      return;
    }

    if (row.column >= row.columns) [[unlikely]] {
      assert(false && "unexpected layout state");
      return;
    }

    // Calculate new filled height.
    row.filledSize.height = std::max(row.filledSize.height, filledHeight);

    LayoutState column;
    column.type = LayoutType::Column;
    column.pos = {row.pos.x + row.filledSize.width, row.pos.y};

    // Calculate column width.
    int const availableWidth = row.size.width - row.filledSize.width;
    int width = 0;
    if (ratioOrWidth > 1.0) {
      width = std::min<int>(ratioOrWidth, availableWidth);
    } else {
      width = std::min<int>(std::round(ratioOrWidth * row.size.width), availableWidth);
    }

    column.size = {width, row.size.height};

    row.filledSize.width += column.size.width;
    row.column += 1;

    pushLayout(column);
  }
}

void rowEnd() {
  auto const layout = []() -> LayoutState& {
    return ctx.layoutStack.back();
  };

  int rowFilledHeight = 0;

  if (layout().type == LayoutType::Column) {
    rowFilledHeight = layout().filledSize.height;
    popLayout();
  }

  if (layout().type != LayoutType::Row) [[unlikely]] {
    assert(false && "unexpected layout state");
    return;
  }

  rowFilledHeight = std::max(rowFilledHeight, layout().filledSize.height);
  popLayout();

  // Update parent layout filled height.
  layout().filledSize.height += rowFilledHeight;
}

void panelBegin(std::string_view title) {
  auto const& style = ctx.style.panel;
  auto const& borderColor = colorAt(ColorID::Border);
  auto const& titleColor = colorAt(ColorID::Text);
  auto const& backgroundColor = colorAt(ColorID::Background);

  auto& parent = ctx.layoutStack.back();
  if (parent.size.width < 2 || parent.size.height < 2) [[unlikely]] {
    return;
  }

  // New layout for panel.
  LayoutState panel;
  panel.type = LayoutType::Container;
  panel.size = {parent.size.width - 2, parent.size.height - parent.filledSize.height - 2};
  panel.pos = {parent.pos.x + 1, parent.pos.y + parent.filledSize.height + 1};

  int titleLength = 0;
  if (title.size() > 0) {
    titleLength = std::min<int>(utf8::stringLength(title), panel.size.width - 2);
  }

  auto cell = draw::cell(style.border.hLine, borderColor, backgroundColor);
  draw::hLine(panel.pos.x, panel.pos.y - 1, panel.size.width + 1, cell);
  cell.ch = style.border.upperLeft;
  draw::point(panel.pos.x - 1, panel.pos.y - 1, cell);
  cell.ch = style.border.upperRight;
  draw::point(panel.pos.x - 1 + panel.size.width + 1, panel.pos.y - 1, cell);

  if (titleLength > 0) {
    draw::text(panel.pos.x + 1, panel.pos.y - 1, title.data(), titleLength, titleColor, backgroundColor);
  }

  pushLayout(panel);
}

void panelEnd() {
  auto const& style = ctx.style.panel;
  auto const& borderColor = colorAt(ColorID::Border);
  auto const& backgroundColor = colorAt(ColorID::Background);

  auto const layout = []() -> LayoutState& {
    return ctx.layoutStack.back();
  };

  if (layout().type != LayoutType::Container) [[unlikely]] {
    assert(false && "unexpected layout state");
    return;
  }

  int const filledHeight = layout().filledSize.height;
  popLayout();

  auto& parent = layout();
  parent.filledSize.height += filledHeight + 2;

  auto cell = draw::cell(style.border.hLine, borderColor, backgroundColor);
  draw::hLine(parent.pos.x + 1, parent.pos.y + parent.filledSize.height - 1, parent.size.width - 2, cell);
  cell.ch = style.border.bottomLeft;
  draw::point(parent.pos.x, parent.pos.y + parent.filledSize.height - 1, cell);
  cell.ch = style.border.bottomRight;
  draw::point(parent.pos.x + parent.size.width - 1, parent.pos.y + parent.filledSize.height - 1, cell);
  cell.ch = style.border.vLine;
  draw::vLine(parent.pos.x, parent.pos.y + parent.filledSize.height - (filledHeight + 1), filledHeight, cell);
  draw::vLine(parent.pos.x + parent.size.width - 1, parent.pos.y + parent.filledSize.height - (filledHeight + 1),
              filledHeight, cell);
}

void label(std::string_view text, Align align) {
  auto const& textColor = colorAt(ColorID::Text);
  auto const& backgroundColor = colorAt(ColorID::Background);

  auto const rect = reserveSpace(1);
  if (rect.width < 1 || rect.height < 1 || text.empty()) [[unlikely]] {
    return;
  }

  int const textLength = std::min<int>(utf8::stringLength(text), rect.width);
  int offsetX = alignWidth(textLength, rect.width, align);

  draw::text(rect.x + offsetX, rect.y, text.data(), textLength, textColor, backgroundColor);
}

void spacer(float ratioOrHeight) {
  if (ratioOrHeight < 0.0) [[unlikely]] {
    ratioOrHeight = 0.0;
  }

  auto& parent = ctx.layoutStack.back();
  int const availableHeight = parent.size.height - parent.filledSize.height;

  int height = 0;
  if (ratioOrHeight > 1.0) {
    height = std::min<int>(ratioOrHeight, availableHeight);
  } else {
    height = std::min<int>(std::round(ratioOrHeight * availableHeight), availableHeight);
  }

  reserveSpace(height);
}

void spinner(std::string_view text, Align align, float& step) {
  auto const& style = ctx.style.spinner;
  auto const& glyphColor = colorAt(ColorID::Glyph);
  auto const& textColor = colorAt(ColorID::Text);
  auto const& backgroundColor = colorAt(ColorID::Background);

  auto const rect = reserveSpace(1);
  if (rect.width < 1 || rect.height < 1 || style.glyphs.empty()) [[unlikely]] {
    return;
  }

  int textLength = 0;
  int innerLength = 1;
  if (text.size() > 0) {
    textLength = std::min<int>(utf8::stringLength(text), rect.width - 2);
    innerLength += (textLength + 1);
  }

  int const offsetX = alignWidth(innerLength, rect.width, align);

  // Spinner
  static constexpr float kSpinInterval = 0.1; // 0.1 seconds
  step += ctx.timeDelta;
  std::size_t const index = std::size_t(std::round(step / kSpinInterval)) % style.glyphs.size();

  // Spinner text
  auto const& cell = draw::cell(style.glyphs[index], glyphColor, backgroundColor);
  draw::point(rect.x + offsetX, rect.y, cell);

  if (textLength > 0) {
    draw::text(rect.x + offsetX + 2, rect.y, text.data(), textLength, textColor, backgroundColor);
  }
}

void progress(float& value) {
  auto const& style = ctx.style.progress;
  auto const& glyphColor = colorAt(ColorID::Glyph);
  auto const& textColor = colorAt(ColorID::Text);
  auto const& backgroundColor = colorAt(ColorID::Background);

  auto const rect = reserveSpace(1);
  if (rect.width < 1 || rect.height < 1) [[unlikely]] {
    return;
  }

  value = std::clamp<float>(value, 0.0, 100.0);
  int const length = std::round((rect.width * value) / 100.0);
  auto const cell = draw::cell(style.barGlyph, glyphColor, backgroundColor);
  draw::hLine(rect.x, rect.y, length, cell);

  char buffer[sizeof(" 100.0% ")];
  std::snprintf(buffer, sizeof(buffer), " %02.1f%% ", double(value));
  std::string_view text(buffer);

  int const textLength = std::min<int>(text.size(), rect.width);
  int const offsetX = alignWidth(textLength, rect.width, Align::Center);
  draw::text(rect.x + offsetX, rect.y, text.data(), textLength, textColor, backgroundColor);
}

bool textInput(std::string& input) {
  auto const& textColor = colorAt(ColorID::Text);
  auto const& backgroundColor = colorAt(ColorID::Background);

  utf8::convert(ctx.inputChars.begin(), ctx.inputChars.end(), std::back_inserter(input));

  if (ctx.pressedKeys[TB_KEY_SPACE]) {
    input.push_back(' ');
  }

  if (ctx.pressedKeys[TB_KEY_BACKSPACE2] && !input.empty()) {
    input.erase(utf8::prev(input.end()), input.end());
  }

  if (ctx.pressedKeys[TB_KEY_CTRL_W]) {
    while (!input.empty() && std::isblank(input.back())) {
      input.pop_back();
    }
    // Remove till first space from back or whole line.
    auto const found = input.rfind(' ');
    if (found == input.npos) {
      input.clear();
    } else {
      input.erase(input.begin() + found + 1, input.end());
    }
  }

  bool const result = ctx.pressedKeys[TB_KEY_ENTER] && input.size() > 0;

  auto const rect = reserveSpace(1);
  if (rect.width < 1 || rect.height < 1) [[unlikely]] {
    return result;
  }

  int const inputLength = utf8::stringLength(input);
  int inputOffset = inputLength - rect.width;
  if (inputOffset < 0) {
    inputOffset = 0;
  }

  draw::text(rect.x, rect.y, input.data(), inputLength, inputOffset, textColor, backgroundColor);
  auto const cell = draw::cell(' ', textColor, backgroundColor);
  draw::hLine(rect.x + (inputLength - inputOffset), rect.y, rect.width - (inputLength + inputOffset), cell);

  return result;
}

class CanvasImpl final : public Canvas {
private:
  static constexpr std::array kPixelMap = {std::array{0x01, 0x08}, std::array{0x02, 0x10}, std::array{0x04, 0x20},
                                           std::array{0x40, 0x80}};
  static constexpr std::uint32_t kBrailleOffset = 0x2800;

  int const x_;
  int const y_;

public:
  CanvasImpl(Rect const& rect) noexcept : Canvas(rect.width * 2, rect.height * 4), x_(rect.x), y_(rect.y) {}

  void point(int x, int y, Color color) override {
    if (x < 0 || x >= width() || y < 0 || y >= height()) {
      return;
    }

    auto& cell = getCellAt(x, y);
    if (cell.ch >= kBrailleOffset) {
      cell.ch |= kPixelMap[y % 4][x % 2];
    } else {
      cell.ch = kBrailleOffset + kPixelMap[y % 4][x % 2];
    }
    cell.fg = static_cast<std::uint16_t>(color);
  }

private:
  ::tb_cell& getCellAt(int x, int y) const noexcept {
    int const posX = x_ + x / 2;
    int const posY = y_ + y / 4;
    return ::tb_cell_buffer()[posY * ::tb_width() + posX];
  }
};

void canvas(float ratioOrHeight, std::function<void(Canvas&)> const& fn) {
  if (ratioOrHeight < 0.0) [[unlikely]] {
    ratioOrHeight = 0.0;
  }

  auto& parent = ctx.layoutStack.back();
  int const availableHeight = parent.size.height - parent.filledSize.height;

  int height = 0;
  if (ratioOrHeight > 1.0) {
    height = std::min<int>(ratioOrHeight, availableHeight);
  } else {
    height = std::min<int>(std::round(ratioOrHeight * availableHeight), availableHeight);
  }

  auto const space = reserveSpace(height);
  CanvasImpl canvasImpl(space);
  std::invoke(fn, canvasImpl);
}

} // namespace xxx
