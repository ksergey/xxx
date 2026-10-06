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

struct rgb_t {
  int r, g, b;
};

// xterm defaults for the 16 scheme colors: only used to pick the nearest one
constexpr rgb_t ansi16[] = {{0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0}, {0, 0, 238}, {205, 0, 205},
    {0, 205, 205}, {229, 229, 229}, {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0}, {92, 92, 255},
    {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
constexpr int cube_levels[] = {0, 95, 135, 175, 215, 255};

[[nodiscard]] constexpr auto distance(rgb_t a, rgb_t b) noexcept -> int {
  return (a.r - b.r) * (a.r - b.r) + (a.g - b.g) * (a.g - b.g) + (a.b - b.b) * (a.b - b.b);
}

[[nodiscard]] constexpr auto palette_rgb(int index) noexcept -> rgb_t {
  if (index < 16) {
    return ansi16[index];
  }
  if (index < 232) {
    auto const i = index - 16;
    return {cube_levels[i / 36], cube_levels[(i / 6) % 6], cube_levels[i % 6]};
  }
  auto const gray = 8 + 10 * (index - 232);
  return {gray, gray, gray};
}

// not std::abs: it isn't constexpr in libc++ 18
[[nodiscard]] constexpr auto absolute(int v) noexcept -> int {
  return v < 0 ? -v : v;
}

[[nodiscard]] constexpr auto nearest_level(int v) noexcept -> int {
  auto best = 0;
  for (int i = 1; i < 6; ++i) {
    if (absolute(cube_levels[i] - v) < absolute(cube_levels[best] - v)) {
      best = i;
    }
  }
  return best;
}

// nearest xterm 256 palette color: the 6x6x6 cube or the gray ramp
[[nodiscard]] constexpr auto to_palette256(rgb_t c) noexcept -> int {
  auto const cube = 16 + 36 * nearest_level(c.r) + 6 * nearest_level(c.g) + nearest_level(c.b);
  auto const average = (c.r + c.g + c.b) / 3;
  auto const gray = 232 + std::clamp((average - 8 + 5) / 10, 0, 23);
  return distance(c, palette_rgb(cube)) <= distance(c, palette_rgb(gray)) ? cube : gray;
}

[[nodiscard]] constexpr auto to_ansi16(rgb_t c) noexcept -> int {
  auto best = 0;
  for (int i = 1; i < 16; ++i) {
    if (distance(c, ansi16[i]) < distance(c, ansi16[best])) {
      best = i;
    }
  }
  return best;
}

static_assert(to_palette256({255, 0, 0}) == 196);
static_assert(to_palette256({128, 128, 128}) == 244);
static_assert(to_ansi16({250, 10, 10}) == 9);

// base is 30 (foreground) or 40 (background)
void append_color(std::string& out, int base, im_color color, ansi_screen::color_mode mode) {
  using mode_t = ansi_screen::color_mode;
  if (color.is_default()) {
    std::format_to(std::back_inserter(out), ";{}", base + 9);
    return;
  }
  auto index = -1; // palette index to emit, or -1 for 24-bit
  if (color.is_indexed()) {
    index = color.index();
    if (index >= 16 && mode == mode_t::ansi16) {
      index = to_ansi16(palette_rgb(index));
    }
  } else {
    auto const c = rgb_t{int((color.value >> 16) & 0xff), int((color.value >> 8) & 0xff), int(color.value & 0xff)};
    if (mode == mode_t::palette256) {
      index = to_palette256(c);
    } else if (mode == mode_t::ansi16) {
      index = to_ansi16(c);
    }
  }
  if (index < 0) {
    std::format_to(std::back_inserter(out), ";{};2;{};{};{}", base + 8, (color.value >> 16) & 0xff,
        (color.value >> 8) & 0xff, color.value & 0xff);
  } else if (index < 8) {
    std::format_to(std::back_inserter(out), ";{}", base + index); // works even on the linux console
  } else if (index < 16) {
    std::format_to(std::back_inserter(out), ";{}", base + 60 + index - 8);
  } else {
    std::format_to(std::back_inserter(out), ";{};5;{}", base + 8, index);
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

void ansi_screen::emit_style(im_style style, std::string& out) {
  if (mode_ == color_mode::none) {
    style.fg = style.bg = im_color::default_tag; // NO_COLOR: attributes only
  }
  if (last_style_known_ && style.attrs == last_style_.attrs) {
    // attributes unchanged: only changed colors
    if (style.fg != last_style_.fg || style.bg != last_style_.bg) {
      out += "\x1b[";
      auto const start = out.size();
      if (style.fg != last_style_.fg) {
        append_color(out, 30, im_color(style.fg), mode_);
      }
      if (style.bg != last_style_.bg) {
        append_color(out, 40, im_color(style.bg), mode_);
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
    if (!im_color(style.fg).is_default()) {
      append_color(out, 30, im_color(style.fg), mode_);
    }
    if (!im_color(style.bg).is_default()) {
      append_color(out, 40, im_color(style.bg), mode_);
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
        if (!last_style_known_ || !(effective(cell.style) == last_style_)) {
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
