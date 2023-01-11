// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#include "ui.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>

#include "impl/data.h"
#include "impl/draw.h"
#include "impl/utf8.h"

namespace xxx {

using namespace xxx::impl;

namespace {

Context ctx;

} // namespace

namespace detail {

int align(int innerWidth, int parentWidth, Align alignment) noexcept {
  if (parentWidth > innerWidth) [[likely]] {
    switch (alignment) {
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

constexpr std::size_t getKeyIndex(std::uint16_t key) noexcept {
  return key <= 0xFF ? key : (((0xFFFF - key) & 0xFF) + 256);
}

// Return current layout.
LayoutState& layout() noexcept {
  return ctx.layoutStack.back();
}

// Set current layout.
void pushLayout(LayoutState const& layout) {
  // Copy parent fillBackground flag to childs (if set).
  bool const fillBackground = detail::layout().fillBackground;
  auto& result = ctx.layoutStack.emplace_back(layout);
  if (fillBackground) [[unlikely]] {
    result.fillBackground = true;
  }
}

// Restore previous layout.
void popLayout() noexcept {
  ctx.layoutStack.pop_back();
}

// Reserve space inside layout.
Rect reserveSpace(int height) noexcept {
  if (height < 0) {
    height = 0;
  }

  auto& layout = detail::layout();

  Rect result;
  result.x = layout.pos.x;
  result.y = layout.pos.y + layout.filledSize.height;
  result.width = layout.size.width;
  result.height = std::min(height, layout.size.height - layout.filledSize.height);

  layout.filledSize.height += result.height;

  if (layout.fillBackground) [[unlikely]] {
    fillRect(result.x, result.y, result.width, result.height, makeCell(' '));
  }

  return result;
}

} // namespace detail

void init() {
  // Init termbox.
  if (int rc = ::tb_init(); rc < 0) {
    std::string reason;
    switch (rc) {
    case TB_EUNSUPPORTED_TERMINAL: {
      reason = "unsuported terminal";
    } break;
    case TB_EFAILED_TO_OPEN_TTY: {
      reason = "failed to open tty";
    } break;
    case TB_EPIPE_TRAP_ERROR: {
      reason = "pipe trap error";
    } break;
    default: {
      reason = "unknown";
    } break;
    }
    throw std::runtime_error{"Failed to init terminal library (" + reason + ")"};
  }

  // Use rbg colors.
  ::tb_select_output_mode(TB_OUTPUT_256);

  // Setup default style.
  ctx.style.panel.titleColor = makeColor(192, 41, 66) | Attribute::Bold;
  ctx.style.panel.borderColor = makeColor(84, 36, 55);
  ctx.style.panel.border = {L'│', L'─', L'╭', L'╮', L'╰', L'╯'};

  ctx.style.spinner.spinnerColor = makeColor(236, 208, 120) | Attribute::Bold;
  ctx.style.spinner.labelColor = Color::Default;
  ctx.style.spinner.glyphs = {{L'⠉', L'⠑', L'⠃', L'⠊', L'⠒', L'⠢', L'⠆', L'⠔', L'⠤', L'⢄', L'⡄', L'⡠',
                               L'⣀', L'⢄', L'⢠', L'⡠', L'⠤', L'⠢', L'⠰', L'⠔', L'⠒', L'⠑', L'⠘', L'⠊'}};

  ctx.style.progress.barColor = makeColor(14, 83, 180);
  ctx.style.progress.labelColor = Color::Default | Attribute::Bold;
  ctx.style.progress.barGlyph = L'│';

  ctx.style.textInput.fg = Color::Default | Attribute::Bold;
  ctx.style.textInput.bg = Color::Default;

  // Preallocate container resources.
  ctx.layoutStack.reserve(8);
  ctx.inputQueueChars.reserve(16);

  // Update clock.
  ctx.timestamp = Clock::now();
}

void shutdown() {
  // Shutdown termbox.
  ::tb_shutdown();
}

bool update(unsigned ms) noexcept {
  ctx.pressedKeys.fill(false);

  ::tb_event event;

  int result = ::tb_peek_event(&event, ms);
  switch (result) {
  case TB_EVENT_KEY: {
    if (event.ch > 0) {
      ctx.inputQueueChars.push_back(event.ch);
    }
    if (event.key > 0) {
      ctx.pressedKeys[detail::getKeyIndex(event.key)] = true;
    }
  } break;
  case TB_EVENT_RESIZE:
  case TB_EVENT_MOUSE:
    break;
  default: {
  } break;
  }

  auto now = Clock::now();
  ctx.deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - ctx.timestamp).count() / 1000.0;
  ctx.timestamp = now;

  return result > 0;
}

bool isKeyPressed(Key k) noexcept {
  return ctx.pressedKeys[detail::getKeyIndex(static_cast<std::uint16_t>(k))];
}

void begin() {
  ctx.screenSize = {::tb_width(), ::tb_height()};
  ctx.layoutStack.clear();

  // Prepare main layout.
  auto& layout = ctx.layoutStack.emplace_back();
  layout.type = LayoutType::Container;
  layout.size = ctx.screenSize;
  layout.pos = {0, 0};
  layout.filledSize = {0, 0};

  // Clear screen.
  ::tb_clear();
}

void end() {
  // Flush to terminal.
  ::tb_present();

  // Clear input chars queue.
  ctx.inputQueueChars.clear();
}

void rowBegin(std::size_t columns) {
  if (columns == 0) {
    return;
  }

  // Parent layout for new row layout.
  auto& parent = detail::layout();

  // New layout.
  LayoutState row;
  row.type = LayoutType::Row;
  row.size = {parent.size.width, parent.size.height - parent.filledSize.height};
  row.pos = {parent.pos.x, parent.pos.y + parent.filledSize.height};
  row.columns = columns;
  row.column = 0;
  row.filledSize = {0, 0};

  // Add layout to stack.
  detail::pushLayout(row);
}

void rowPush(float ratioOrWidth) {
  if (ratioOrWidth < 0.0) [[unlikely]] {
    ratioOrWidth = 0.0;
  }

  if (detail::layout().type == LayoutType::Row) {
    auto& row = detail::layout();

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
    detail::pushLayout(column);
  } else if (detail::layout().type == LayoutType::Column) {
    int const filledHeight = detail::layout().filledSize.height;
    // Remove prev column layout.
    detail::popLayout();

    // Parent row layout.
    auto& row = detail::layout();

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

    detail::pushLayout(column);
  }
}

void rowEnd() {
  int rowFilledHeight = 0;

  if (detail::layout().type == LayoutType::Column) {
    rowFilledHeight = detail::layout().filledSize.height;
    detail::popLayout();
  }

  if (detail::layout().type != LayoutType::Row) [[unlikely]] {
    assert(false && "unexpected layout state");
    return;
  }

  rowFilledHeight = std::max(rowFilledHeight, detail::layout().filledSize.height);
  detail::popLayout();

  // Update parent layout filled height.
  detail::layout().filledSize.height += rowFilledHeight;
}

void panelBegin(std::string_view title) {
  auto& parent = detail::layout();

  if (parent.size.width < 2 || parent.size.height < 2) [[unlikely]] {
    // TODO: may be this is wrong
    return;
  }

  // New layout for panel.
  LayoutState panel;
  panel.type = LayoutType::Container;
  panel.size = {parent.size.width - 2, parent.size.height - parent.filledSize.height - 2};
  panel.pos = {parent.pos.x + 1, parent.pos.y + parent.filledSize.height + 1};

  int titleLength = 0;
  if (title.size() > 0) {
    titleLength = std::min<int>(utf8_string_length(title), panel.size.width - 2);
  }

  auto cell = makeCell(ctx.style.panel.border.horizontalLine, ctx.style.panel.borderColor);
  drawHorizontalLine(panel.pos.x, panel.pos.y - 1, panel.size.width + 1, cell);
  cell.ch = ctx.style.panel.border.upperLeftCorner;
  drawCell(panel.pos.x - 1, panel.pos.y - 1, cell);
  cell.ch = ctx.style.panel.border.upperRightCorner;
  drawCell(panel.pos.x - 1 + panel.size.width + 1, panel.pos.y - 1, cell);

  if (titleLength > 0) {
    drawText(panel.pos.x + 1, panel.pos.y - 1, title.data(), titleLength, ctx.style.panel.titleColor);
  }

  detail::pushLayout(panel);
}

void panelEnd() {
  if (detail::layout().type != LayoutType::Container) [[unlikely]] {
    assert(false && "unexpected layout state");
    return;
  }

  int const filledHeight = detail::layout().filledSize.height;
  detail::popLayout();

  auto& parent = detail::layout();
  parent.filledSize.height += filledHeight + 2;

  auto cell = makeCell(ctx.style.panel.border.horizontalLine, ctx.style.panel.borderColor);
  drawHorizontalLine(parent.pos.x + 1, parent.pos.y + parent.filledSize.height - 1, parent.size.width - 2, cell);
  cell.ch = ctx.style.panel.border.bottomLeftCorner;
  drawCell(parent.pos.x, parent.pos.y + parent.filledSize.height - 1, cell);
  cell.ch = ctx.style.panel.border.bottomRightCorner;
  drawCell(parent.pos.x + parent.size.width - 1, parent.pos.y + parent.filledSize.height - 1, cell);
  cell.ch = ctx.style.panel.border.verticalLine;
  drawVerticalLine(parent.pos.x, parent.pos.y + parent.filledSize.height - (filledHeight + 1), filledHeight, cell);
  drawVerticalLine(parent.pos.x + parent.size.width - 1, parent.pos.y + parent.filledSize.height - (filledHeight + 1),
                   filledHeight, cell);
}

void fixedPanelBegin(Rect const& geom, std::string_view title) {
  LayoutState layout;
  layout.type = LayoutType::Container;
  layout.size = {geom.width, geom.height};
  layout.pos = {geom.x, geom.y};
  layout.fillBackground = true;
  detail::pushLayout(layout);

  panelBegin(title);
}

void fixedPanelEnd() {
  panelEnd();

  if (detail::layout().type != LayoutType::Container) [[unlikely]] {
    assert(false && "unexpected layout state");
    return;
  }

  detail::popLayout();
}

void spacer(float ratioOrHeight) {
  if (ratioOrHeight < 0.0) [[unlikely]] {
    ratioOrHeight = 0.0;
  }

  auto& parent = detail::layout();
  int availableHeight = parent.size.height - parent.filledSize.height;

  int height = 0;
  if (ratioOrHeight > 1.0) {
    height = std::min<int>(ratioOrHeight, availableHeight);
  } else {
    height = std::min<int>(std::round(ratioOrHeight * availableHeight), availableHeight);
  }

  detail::reserveSpace(height);
}

void label(std::string_view text, Color color, Align alignment) {
  auto rect = detail::reserveSpace(1);
  if (rect.width < 1 || rect.height < 1 || text.empty()) [[unlikely]] {
    return;
  }

  int strLength = std::min<int>(utf8_string_length(text), rect.width);
  int offsetX = detail::align(strLength, rect.width, alignment);
  drawText(rect.x + offsetX, rect.y, text.data(), strLength, color);
}

void spinner(float& step, std::string_view text, Align alignment) {
  auto rect = detail::reserveSpace(1);
  if (rect.width < 1 || rect.height < 1 || ctx.style.spinner.glyphs.empty()) [[unlikely]] {
    assert(!ctx.style.spinner.glyphs.empty() && "spinner frames not configured");
    return;
  }

  int strLength = 0;
  int innerLength = 1;
  if (text.size() > 0) {
    strLength = std::min<int>(utf8_string_length(text), rect.width - 2);
    innerLength += (strLength + 1);
  }

  int offsetX = detail::align(innerLength, rect.width, alignment);

  // Spinner
  static constexpr float kSpinInterval = 0.1; // 0.1 seconds
  step += ctx.deltaTime;
  std::size_t const index = std::size_t(std::round(step / kSpinInterval)) % ctx.style.spinner.glyphs.size();

  // Spinner text
  drawCell(rect.x + offsetX, rect.y, makeCell(ctx.style.spinner.glyphs[index], ctx.style.spinner.spinnerColor));

  if (strLength > 0) {
    drawText(rect.x + offsetX + 2, rect.y, text.data(), strLength, ctx.style.spinner.labelColor);
  }
}

void progress(float& value) {
  auto rect = detail::reserveSpace(1);
  if (rect.width < 1 || rect.height < 1) [[unlikely]] {
    return;
  }

  // Bar
  value = std::clamp<float>(value, 0.0, 100.0);
  int const length = std::round((rect.width * value) / 100.0);
  auto const cell = makeCell(ctx.style.progress.barGlyph, ctx.style.progress.barColor);
  drawHorizontalLine(rect.x, rect.y, length, cell);

  // Text
  char buffer[sizeof(" 100.0% ")];
  std::snprintf(buffer, sizeof(buffer), " %02.1f%% ", double(value));
  std::string_view text(buffer);

  int strLength = std::min<int>(text.size(), rect.width);
  int offsetX = detail::align(strLength, rect.width, Align::Center);
  drawText(rect.x + offsetX, rect.y, text.data(), strLength, ctx.style.progress.labelColor);
}

bool textInput(std::string& input) {
  u32_to_utf8(ctx.inputQueueChars.begin(), ctx.inputQueueChars.end(), std::back_inserter(input));

  if (ctx.pressedKeys[TB_KEY_SPACE]) {
    input.push_back(' ');
  }

  if (ctx.pressedKeys[TB_KEY_BACKSPACE2] && !input.empty()) {
    input.erase(utf8_prev(input.end()), input.end());
  }

  if (ctx.pressedKeys[TB_KEY_CTRL_W]) {
    while (!input.empty() && std::isblank(input.back())) {
      input.pop_back();
    }
    // Remove till first space from back or whole line.
    auto found = input.rfind(' ');
    if (found == input.npos) {
      input.clear();
    } else {
      input.erase(input.begin() + found + 1, input.end());
    }
  }

  bool result = ctx.pressedKeys[TB_KEY_ENTER] && input.size() > 0;

  auto rect = detail::reserveSpace(1);
  if (rect.width < 1 || rect.height < 1) [[unlikely]] {
    return result;
  }

  int inputLength = utf8_string_length(input);
  int inputOffset = inputLength - (rect.width - 1);
  if (inputOffset < 0) {
    inputOffset = 0;
  }

  drawText(rect.x, rect.y, input.data(), inputLength, inputOffset, ctx.style.textInput.fg, ctx.style.textInput.bg);
  drawCell(rect.x + (inputLength - inputOffset), rect.y, makeCell(' ', ctx.style.textInput.bg, ctx.style.textInput.fg));

  return result;
}

} // namespace xxx
