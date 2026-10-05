// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "ansi_screen.h"

#include <algorithm>
#include <format>
#include <iterator>

#include "unicode.h"

namespace xxx {

namespace {

// never produced by set_cell: front cell with it always differs from back
constexpr auto invalid_ch = std::uint32_t(0xffffffff);

[[nodiscard]] constexpr auto same(im_cell const& a, im_cell const& b) noexcept -> bool {
  return a.ch == b.ch && a.style == b.style;
}

void append_color(std::string& out, int base, std::uint32_t color) {
  if (color == 0) {
    std::format_to(std::back_inserter(out), ";{}", base + 9); // terminal default
  } else {
    std::format_to(std::back_inserter(out), ";{};2;{};{};{}", base + 8, (color >> 16) & 0xff, (color >> 8) & 0xff,
        color & 0xff);
  }
}

} // namespace

void ansi_screen::resize(im_vec2 size) {
  size_ = im_vec2(std::max(0, size.x), std::max(0, size.y));
  auto const n = std::size_t(size_.x) * std::size_t(size_.y);
  back_.assign(n, im_cell{.ch = ' ', .style = {}});
  front_.assign(n, im_cell{.ch = invalid_ch, .style = {}});
  last_style_known_ = false;
}

void ansi_screen::invalidate() {
  std::fill(front_.begin(), front_.end(), im_cell{.ch = invalid_ch, .style = {}});
  last_style_known_ = false;
}

void ansi_screen::clear(im_style const& style) {
  std::fill(back_.begin(), back_.end(), im_cell{.ch = ' ', .style = style});
}

void ansi_screen::set_cell(int x, int y, std::uint32_t ch, im_style const& style) {
  if (x < 0 || y < 0 || x >= size_.x || y >= size_.y) {
    return;
  }
  back_[std::size_t(y * size_.x + x)] = im_cell{.ch = ch, .style = style};
}

void ansi_screen::emit_style(im_style const& style, std::string& out) {
  if (last_style_known_ && style.attrs == last_style_.attrs) {
    // attributes unchanged: only changed colors
    if (style.fg != last_style_.fg || style.bg != last_style_.bg) {
      out += "\x1b[";
      auto const start = out.size();
      if (style.fg != last_style_.fg) {
        append_color(out, 30, style.fg);
      }
      if (style.bg != last_style_.bg) {
        append_color(out, 40, style.bg);
      }
      out.erase(start, 1); // leading ';'
      out += 'm';
    }
  } else {
    // reset, then attributes and both colors
    out += "\x1b[0";
    static constexpr std::pair<std::uint32_t, char const*> attrs[] = {{im_attr_bold, ";1"}, {im_attr_dim, ";2"},
        {im_attr_italic, ";3"}, {im_attr_underline, ";4"}, {im_attr_blink, ";5"}, {im_attr_reverse, ";7"},
        {im_attr_strikeout, ";9"}};
    for (auto const& [flag, code] : attrs) {
      if (style.attrs & flag) {
        out += code;
      }
    }
    if (style.fg != 0) {
      append_color(out, 30, style.fg);
    }
    if (style.bg != 0) {
      append_color(out, 40, style.bg);
    }
    out += 'm';
  }
  last_style_ = style;
  last_style_known_ = true;
}

void ansi_screen::present(std::string& out) {
  auto const start = out.size();
  auto cursor = im_vec2(-1, -1); // unknown: first write always moves explicitly
  char utf8[4];

  for (int y = 0; y < size_.y; ++y) {
    for (int x = 0; x < size_.x;) {
      auto const i = std::size_t(y * size_.x + x);
      auto cell = back_[i];
      auto width = char_width(cell.ch);
      if (width == 2 && x == size_.x - 1) {
        cell.ch = ' '; // wide char doesn't fit into the last column
        width = 1;
      }

      if (!same(cell, front_[i])) {
        // wide char replaced by narrow one: its right half must be repainted too
        if (front_[i].ch != invalid_ch && char_width(front_[i].ch) == 2 && width == 1 && x + 1 < size_.x) {
          front_[i + 1].ch = invalid_ch;
        }
        if (cursor != im_vec2(x, y)) {
          std::format_to(std::back_inserter(out), "\x1b[{};{}H", y + 1, x + 1);
        }
        if (!last_style_known_ || !(cell.style == last_style_)) {
          emit_style(cell.style, out);
        }
        out.append(utf8, utf8_encode(cell.ch, utf8));
        front_[i] = cell;
        cursor = im_vec2(x + width, y);
      }
      // cell under the right half of a wide char is not drawn while the wide char stays
      x += width;
    }
  }

  if (out.size() != start) {
    out.insert(start, "\x1b[?2026h");
    out += "\x1b[?2026l";
  }
}

} // namespace xxx
