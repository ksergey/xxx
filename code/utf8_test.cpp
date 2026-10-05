// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

#include "im_renderer.h"
#include "unicode.h"
#include "unicode_width_table.h"

namespace xxx::testing {

using namespace std::string_view_literals;

namespace {

auto decode_all(std::string_view s) -> std::vector<std::uint32_t> {
  auto result = std::vector<std::uint32_t>();
  for_each_codepoint(s, [&](std::uint32_t ch) { result.push_back(ch); });
  return result;
}

constexpr auto fffd = replacement_char;

} // namespace

TEST_SUITE("utf8_decode") {

  TEST_CASE("valid sequences of every length") {
    CHECK(decode_all("a") == std::vector<std::uint32_t>{'a'});
    CHECK(decode_all("ж") == std::vector<std::uint32_t>{0x436});
    CHECK(decode_all("€") == std::vector<std::uint32_t>{0x20ac});
    CHECK(decode_all("😀") == std::vector<std::uint32_t>{0x1f600});
    CHECK(decode_all("\xf4\x8f\xbf\xbf"sv) == std::vector<std::uint32_t>{0x10ffff});
  }

  TEST_CASE("consumed length") {
    std::uint32_t ch = 0;
    CHECK(utf8_decode("€x", ch) == 3);
    CHECK(ch == 0x20ac);
  }

  TEST_CASE("stop rules kept from previous implementation") {
    std::uint32_t ch = 0;
    CHECK(utf8_decode("", ch) == 0);
    CHECK(utf8_decode("\0a"sv, ch) == 0);                 // NUL stops
    CHECK(decode_all("a\xe2\x82"sv) == std::vector<std::uint32_t>{'a'}); // truncated tail dropped
  }

  TEST_CASE("invalid bytes become U+FFFD and resync") {
    CHECK(decode_all("\x80"sv) == std::vector<std::uint32_t>{fffd});                // stray continuation
    CHECK(decode_all("\xf8\x88\x80\x80\x80"sv).front() == fffd);                  // 5-byte lead
    CHECK(decode_all("\xc0\xaf"sv) == std::vector<std::uint32_t>{fffd, fffd});      // overlong '/'
    CHECK(decode_all("\xed\xa0\x80"sv) == std::vector<std::uint32_t>{fffd, fffd, fffd}); // surrogate
    CHECK(decode_all("\xf4\x90\x80\x80"sv).front() == fffd);                      // > U+10FFFF
    // broken continuation: lead is replaced, decoding continues with the next byte
    CHECK(decode_all("\xe2(\xa1x"sv) == std::vector<std::uint32_t>{fffd, '(', fffd, 'x'});
  }
}

TEST_SUITE("utf8_encode") {

  TEST_CASE("length boundaries") {
    char b[4];
    CHECK(utf8_encode(0x7f, b) == 1);
    CHECK(utf8_encode(0x80, b) == 2);
    CHECK(utf8_encode(0x7ff, b) == 2);
    CHECK(utf8_encode(0x800, b) == 3);
    CHECK(utf8_encode(0xffff, b) == 3);
    CHECK(utf8_encode(0x10000, b) == 4);
    CHECK(utf8_encode(0x10ffff, b) == 4);
  }

  TEST_CASE("invalid codepoints are encoded as U+FFFD") {
    char b[4];
    CHECK(std::string_view(b, utf8_encode(0xd800, b)) == "\xef\xbf\xbd"sv);
    CHECK(std::string_view(b, utf8_encode(0x110000, b)) == "\xef\xbf\xbd"sv);
  }

  TEST_CASE("roundtrip of every valid codepoint") {
    char b[4];
    auto failures = 0;
    for (std::uint32_t cp = 1; cp <= 0x10ffff; ++cp) {
      if (cp >= 0xd800 && cp <= 0xdfff) {
        continue;
      }
      auto const n = utf8_encode(cp, b);
      std::uint32_t back = 0;
      if (utf8_decode(std::string_view(b, n), back) != n || back != cp) {
        ++failures;
      }
    }
    CHECK(failures == 0);
  }
}

TEST_SUITE("width table") {

  TEST_CASE("sorted and non-overlapping") {
    auto const& t = detail::wide_ranges;
    auto ok = true;
    for (std::size_t i = 0; i < t.size(); ++i) {
      ok = ok && t[i].first <= t[i].last && (i == 0 || t[i - 1].last < t[i].first);
    }
    CHECK(ok);
  }

  TEST_CASE("range boundaries") {
    CHECK(char_width(0x10ff) == 1);
    CHECK(char_width(0x1100) == 2); // Hangul Jamo start
    CHECK(char_width(0x115f) == 2);
    CHECK(char_width(0x1160) == 1);
  }

  TEST_CASE("recent Unicode additions are wide") {
    CHECK(char_width(0x1f1ae) == 2); // emoji added in Unicode 15+
    CHECK(char_width(0x16ff2) == 2); // ideographic, Unicode 17
  }

  TEST_CASE("ambiguous width is narrow") {
    CHECK(char_width(0x3248) == 1); // circled number on black square
    CHECK(char_width(0x2460) == 1); // circled digit one
  }
}

TEST_SUITE("im_style") {

  TEST_CASE("attributes combine") {
    auto const s = im_style(im_color(0x112233u), im_color(0x445566u)).with_reverse().with_underline();
    CHECK(s.fg == 0x112233u);
    CHECK(s.bg == 0x445566u);
    CHECK(s.attrs == (im_attr_reverse | im_attr_underline));
    CHECK(s.with_attrs(im_attr_bold).attrs == (im_attr_reverse | im_attr_underline | im_attr_bold));
  }

  TEST_CASE("attributes do not touch colors") {
    auto const s = im_style(im_color(0xffffffu)).with_blink();
    CHECK(s.fg == 0xffffffu);
    CHECK(s == im_style(im_color(0xffffffu), im_color(), im_attr_blink));
  }
}

} // namespace xxx::testing
