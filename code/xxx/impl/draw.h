// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#pragma once

#include <memory>

#include <xxx/ui.h>

#include <termbox.h>

#include "utf8.h"

namespace xxx::impl {

using Cell = ::tb_cell;

inline Cell makeCell(std::uint32_t ch, Color fg = Color::Default, Color bg = Color::Default) noexcept {
  return Cell{ch, static_cast<std::uint16_t>(fg), static_cast<std::uint16_t>(bg)};
}

inline void drawCell(int x, int y, Cell const& cell) noexcept {
  ::tb_put_cell(x, y, &cell);
}

inline void drawHorizontalLine(int x, int y, int length, Cell const& cell) noexcept {
  while (length-- > 0) {
    drawCell(x, y, cell);
    x += 1;
  }
}

inline void drawVerticalLine(int x, int y, int length, Cell const& cell) noexcept {
  while (length-- > 0) {
    drawCell(x, y, cell);
    y += 1;
  }
}

inline void fillRect(int x, int y, int width, int height, Cell const& cell) noexcept {
  while (height-- > 0) {
    drawHorizontalLine(x, y, width, cell);
    y += 1;
  }
}

/// Draw text. Cheaper to pass offset to function instead of trim `str` from start.
inline void drawText(int x, int y, char const* str, int length, int offset, Color fg = Color::Default,
                     Color bg = Color::Default) {
  if (offset < 0) {
    offset = 0;
  }

  auto cell = makeCell(' ', fg, bg);
  auto it = make_utf8_iterator(str);

  if (offset > 0) {
    while (length > 0 && offset > 0) {
      --length;
      --offset;
      ++it;
    }
  }

  while (length > 0) {
    cell.ch = *it++;
    drawCell(x++, y, cell);
    length -= 1;
  }
}

inline void drawText(int x, int y, char const* str, int length, Color fg = Color::Default, Color bg = Color::Default) {
  return drawText(x, y, str, length, 0, fg, bg);
}

} // namespace xxx::impl
