// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

#include "unicode.h"

namespace xxx::testing {

using namespace std::string_view_literals;

namespace {

auto to_vec(std::string_view s) -> std::vector<std::uint32_t> {
  auto const span = utf8_to_unicode(s);
  return {span.begin(), span.end()};
}

} // namespace

TEST_SUITE("unicode") {

  TEST_CASE("ascii") {
    CHECK(to_vec("abc") == std::vector<std::uint32_t>{'a', 'b', 'c'});
  }

  TEST_CASE("multibyte") {
    CHECK(to_vec("ж") == std::vector<std::uint32_t>{U'ж'});         // 2 bytes
    CHECK(to_vec("€") == std::vector<std::uint32_t>{U'€'});         // 3 bytes
    CHECK(to_vec("😀") == std::vector<std::uint32_t>{U'\U0001F600'}); // 4 bytes
    CHECK(to_vec("aж€😀").size() == 4);
  }

  TEST_CASE("empty") {
    CHECK(utf8_to_unicode(""sv).empty());
    CHECK(unicode_to_utf8(std::span<std::uint32_t const>()).empty());
  }

  TEST_CASE("stops at embedded NUL") {
    CHECK(to_vec("ab\0cd"sv).size() == 2);
  }

  TEST_CASE("truncated sequence is dropped") {
    // first 2 bytes of 3-byte '€'
    CHECK(to_vec("a\xe2\x82"sv) == std::vector<std::uint32_t>{'a'});
  }

  TEST_CASE("roundtrip") {
    for (auto const s : {"hello"sv, "привет мир"sv, "日本語"sv, "mixed: aж€😀"sv}) {
      CAPTURE(s);
      auto const cps = to_vec(s);
      CHECK(unicode_to_utf8(cps) == s);

      std::string out;
      unicode_to_utf8(cps, out);
      CHECK(out == s);

      std::vector<std::uint32_t> cps2;
      utf8_to_unicode(s, cps2);
      CHECK(cps2 == cps);
    }
  }

  TEST_CASE("char_width") {
    CHECK(char_width('a') == 1);
    CHECK(char_width(U'ж') == 1);
    CHECK(char_width(U'€') == 1);
    CHECK(char_width(U'⣿') == 1); // braille, used by canvas/spinner
    CHECK(char_width(U'╭') == 1); // box drawing, used by borders
    CHECK(char_width(U'日') == 2);
    CHECK(char_width(U'Ａ') == 2); // fullwidth latin
    CHECK(char_width(U'\U0001F600') == 2);
    // zero-width and control chars take one cell in termbox2
    CHECK(char_width(0x0301) == 1);
    CHECK(char_width('\t') == 1);
  }

  TEST_CASE("text_width") {
    CHECK(text_width(to_vec("")) == 0);
    CHECK(text_width(to_vec("hello")) == 5);
    CHECK(text_width(to_vec("привет")) == 6);
    CHECK(text_width(to_vec("日本語")) == 6);
    CHECK(text_width(to_vec("a日😀b")) == 6);
  }

  TEST_CASE("slice_columns") {
    // "a日本b": columns  a=0, 日=1..2, 本=3..4, b=5
    auto const text = to_vec("a日本b");
    auto const span = std::span<std::uint32_t const>(text);

    auto const check = [&](int skip, int max_width, std::size_t first, std::size_t count, int pad_left, int pad_right) {
      CAPTURE(skip);
      CAPTURE(max_width);
      auto const s = slice_columns(span, skip, max_width);
      CHECK(s.text.data() == span.data() + first);
      CHECK(s.text.size() == count);
      CHECK(s.pad_left == pad_left);
      CHECK(s.pad_right == pad_right);
      CHECK(s.width() <= max_width);
    };

    SUBCASE("whole text") {
      check(0, 100, 0, 4, 0, 0);
      check(0, 6, 0, 4, 0, 0);
    }
    SUBCASE("cut right on char boundary") {
      check(0, 3, 0, 2, 0, 0);
    }
    SUBCASE("cut right inside wide char") {
      check(0, 2, 0, 1, 0, 1); // "a" + pad
      check(0, 4, 0, 2, 0, 1); // "a日" + pad
    }
    SUBCASE("skip on char boundary") {
      check(1, 100, 1, 3, 0, 0);
      check(3, 100, 2, 2, 0, 0);
    }
    SUBCASE("skip inside wide char") {
      check(2, 100, 2, 2, 1, 0); // pad + "本b"
      check(4, 100, 3, 1, 1, 0); // pad + "b"
    }
    SUBCASE("both edges inside wide chars") {
      check(2, 2, 2, 0, 1, 1); // pad + pad
    }
    SUBCASE("single column inside wide char") {
      check(2, 1, 2, 0, 1, 0);
    }
    SUBCASE("past the end") {
      CHECK(slice_columns(span, 6, 10).empty());
      CHECK(slice_columns(span, 100, 10).empty());
    }
    SUBCASE("non-positive width") {
      CHECK(slice_columns(span, 0, 0).empty());
      CHECK(slice_columns(span, 0, -1).empty());
    }
    SUBCASE("negative skip is clamped") {
      check(-5, 100, 0, 4, 0, 0);
    }
  }

  TEST_CASE("output overloads overwrite previous contents") {
    std::vector<std::uint32_t> v{1, 2, 3};
    utf8_to_unicode("x", v);
    CHECK(v == std::vector<std::uint32_t>{'x'});

    std::string s = "garbage";
    unicode_to_utf8(std::vector<std::uint32_t>{'y'}, s);
    CHECK(s == "y");
  }
}

} // namespace xxx::testing
