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
