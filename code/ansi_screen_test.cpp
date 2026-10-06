// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>

#include <doctest/doctest.h>

#include "ansi_screen.h"

namespace xxx::testing {

namespace {

auto present(ansi_screen& s) -> std::string {
  auto out = std::string();
  s.present(out);
  return out;
}

// wrap expected frame body into synchronized output markers
auto frame(std::string const& body) -> std::string {
  return "\x1b[?2026h" + body + "\x1b[?2026l";
}

void put(ansi_screen& s, int x, int y, std::string_view text, im_style const& style = {}) {
  for (auto const ch : text) {
    s.set_cell(x++, y, std::uint32_t(ch), style);
  }
}

} // namespace

TEST_SUITE("ansi screen") {

  TEST_CASE("first frame draws everything") {
    ansi_screen s;
    s.resize(im_vec2(3, 1));
    s.clear({});
    put(s, 0, 0, "ab");
    CHECK(present(s) == frame("\x1b[1;1H\x1b[0mab "));
  }

  TEST_CASE("unchanged frame sends nothing") {
    ansi_screen s;
    s.resize(im_vec2(3, 2));
    s.clear({});
    present(s);
    s.clear({});
    CHECK(present(s).empty());
  }

  TEST_CASE("only changed cells, cursor moved once for a run") {
    ansi_screen s;
    s.resize(im_vec2(5, 2));
    s.clear({});
    present(s);
    s.clear({});
    put(s, 1, 1, "xyz");
    CHECK(present(s) == frame("\x1b[2;2Hxyz")); // style is already default: no SGR
  }

  TEST_CASE("separate changes get separate cursor moves") {
    ansi_screen s;
    s.resize(im_vec2(5, 2));
    s.clear({});
    present(s);
    s.clear({});
    s.set_cell(0, 0, 'a', {});
    s.set_cell(4, 1, 'b', {});
    CHECK(present(s) == frame("\x1b[1;1Ha\x1b[2;5Hb"));
  }

  TEST_CASE("color change sends only colors, attribute change resets") {
    ansi_screen s;
    s.resize(im_vec2(3, 1));
    s.clear({});
    present(s);
    s.clear({});
    s.set_cell(0, 0, 'a', im_style(im_color(0x112233u)));
    s.set_cell(1, 0, 'b', im_style(im_color(0x112233u), im_color(0xff0000u)));
    s.set_cell(2, 0, 'c', im_style(im_color(0x112233u)).with_reverse());
    CHECK(present(s) == frame("\x1b[1;1H"
                              "\x1b[38;2;17;34;51ma"
                              "\x1b[48;2;255;0;0mb"
                              "\x1b[0;7;38;2;17;34;51mc"));
  }

  TEST_CASE("back to default colors") {
    // 1 cell: the colored cell is the last one written, terminal stays in its colors
    ansi_screen s;
    s.resize(im_vec2(1, 1));
    s.clear({});
    s.set_cell(0, 0, 'a', im_style(im_color(0x010203u), im_color(0x040506u)));
    present(s);
    s.clear({});
    s.set_cell(0, 0, 'a', {});
    CHECK(present(s) == frame("\x1b[1;1H\x1b[39;49ma"));
  }

  TEST_CASE("all attributes") {
    ansi_screen s;
    s.resize(im_vec2(1, 1));
    s.clear(im_style(im_color(), im_color(),
        im_attr_bold | im_attr_dim | im_attr_italic | im_attr_underline | im_attr_blink | im_attr_reverse |
            im_attr_strikeout));
    CHECK(present(s) == frame("\x1b[1;1H\x1b[0;1;2;3;4;5;7;9m "));
  }

  TEST_CASE("style already active in terminal is not repeated") {
    ansi_screen s;
    s.resize(im_vec2(2, 1));
    s.clear({});
    s.set_cell(0, 0, 'a', im_style(im_color(0x010203u)));
    present(s); // ends with the default colored space: terminal is in default style
    s.clear({});
    s.set_cell(0, 0, 'b', {});
    CHECK(present(s) == frame("\x1b[1;1Hb"));
  }

  TEST_CASE("utf8 output") {
    ansi_screen s;
    s.resize(im_vec2(2, 1));
    s.clear({});
    s.set_cell(0, 0, 0x436, {}); // ж
    CHECK(present(s) == frame("\x1b[1;1H\x1b[0m\xd0\xb6 "));
  }

  TEST_CASE("wide char covers next cell") {
    ansi_screen s;
    s.resize(im_vec2(4, 1));
    s.clear({});
    s.set_cell(0, 0, 0x65e5, {}); // 日
    s.set_cell(1, 0, 'x', {});    // hidden under the wide char
    CHECK(present(s) == frame("\x1b[1;1H\x1b[0m\xe6\x97\xa5  "));

    // changing the hidden cell while the wide char stays sends nothing
    s.clear({});
    s.set_cell(0, 0, 0x65e5, {});
    s.set_cell(1, 0, 'y', {});
    CHECK(present(s).empty());
  }

  TEST_CASE("wide char replaced by narrow: right half is repainted") {
    ansi_screen s;
    s.resize(im_vec2(3, 1));
    s.clear({});
    s.set_cell(0, 0, 0x65e5, {});
    present(s);
    s.clear({});
    s.set_cell(0, 0, 'a', {});
    CHECK(present(s) == frame("\x1b[1;1Ha "));
  }

  TEST_CASE("wide char in the last column becomes a space") {
    ansi_screen s;
    s.resize(im_vec2(2, 1));
    s.clear({});
    s.set_cell(0, 0, 'a', {});
    s.set_cell(1, 0, 0x65e5, {});
    CHECK(present(s) == frame("\x1b[1;1H\x1b[0ma "));
  }

  TEST_CASE("resize and invalidate redraw everything") {
    ansi_screen s;
    s.resize(im_vec2(2, 1));
    s.clear({});
    present(s);
    s.invalidate();
    s.clear({});
    CHECK(present(s) == frame("\x1b[1;1H\x1b[0m  "));
    s.resize(im_vec2(1, 2));
    s.clear({});
    CHECK(present(s) == frame("\x1b[1;1H\x1b[0m \x1b[2;1H "));
  }

  TEST_CASE("out of screen cells are ignored") {
    ansi_screen s;
    s.resize(im_vec2(1, 1));
    s.clear({});
    present(s);
    s.set_cell(-1, 0, 'x', {});
    s.set_cell(1, 0, 'x', {});
    s.set_cell(0, 1, 'x', {});
    CHECK(present(s).empty());
  }
}

} // namespace xxx::testing

namespace xxx::testing {

TEST_SUITE("ansi screen: colors") {

  auto one_cell = [](ansi_screen::color_mode mode, im_style const& style) {
    ansi_screen s;
    s.set_color_mode(mode);
    s.resize(im_vec2(1, 1));
    s.clear(style);
    auto out = std::string();
    s.present(out);
    return out;
  };
  using mode = ansi_screen::color_mode;

  TEST_CASE("palette colors use the short classic codes") {
    CHECK(one_cell(mode::truecolor, im_style(ansi::red)) == "\x1b[?2026h\x1b[1;1H\x1b[0;31m \x1b[?2026l");
    CHECK(one_cell(mode::truecolor, im_style(ansi::bright_cyan, ansi::blue)).find("\x1b[0;96;44m") != std::string::npos);
    CHECK(one_cell(mode::truecolor, im_style(im_color::indexed(200))).find(";38;5;200m") != std::string::npos);
  }

  TEST_CASE("black is black, not the terminal default") {
    CHECK(one_cell(mode::truecolor, im_style(im_color(0x000000u))).find(";38;2;0;0;0m") != std::string::npos);
  }

  TEST_CASE("24-bit colors are mapped to what the terminal shows") {
    auto const red = im_style(im_color(0xff0000u));
    CHECK(one_cell(mode::truecolor, red).find(";38;2;255;0;0m") != std::string::npos);
    CHECK(one_cell(mode::palette256, red).find(";38;5;196m") != std::string::npos);
    CHECK(one_cell(mode::ansi16, red).find("\x1b[0;91m") != std::string::npos);
    // palette above 15 in a 16 color terminal
    CHECK(one_cell(mode::ansi16, im_style(im_color::indexed(196))).find("\x1b[0;91m") != std::string::npos);
  }

  TEST_CASE("NO_COLOR mode: attributes only") {
    auto const out = one_cell(mode::none, im_style(ansi::red, ansi::blue, im_attr_reverse));
    CHECK(out == "\x1b[?2026h\x1b[1;1H\x1b[0;7m \x1b[?2026l");
  }
}

} // namespace xxx::testing
