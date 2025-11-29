// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "unicode.h"

#include <termbox2.h>

namespace xxx {

auto utf8_to_unicode(std::string_view input) -> std::span<std::uint32_t const> {
  thread_local std::vector<std::uint32_t> cache;

  cache.resize(input.size());

  char const* begin = input.data();
  char const* end = begin + input.size();
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

void utf8_to_unicode(std::string_view input, std::vector<std::uint32_t>& output) {
  output.clear();

  char const* begin = input.data();
  char const* end = begin + input.size();

  while (begin < end) {
    if (*begin == '\0') {
      break;
    }
    auto const length = ::tb_utf8_char_length(*begin);
    if (begin + length > end) [[unlikely]] {
      break;
    }
    ::tb_utf8_char_to_unicode(&output.emplace_back(), begin);
    begin += length;
  }
}

[[nodiscard]] auto unicode_to_utf8(std::span<std::uint32_t const> input) -> std::string_view {
  thread_local std::string cache;
  cache.clear();
  char codepoint[7];
  for (auto ch : input) {
    ::tb_utf8_unicode_to_char(codepoint, ch);
    cache.append(codepoint);
  }
  return cache;
}

void unicode_to_utf8(std::span<std::uint32_t const> input, std::string& output) {
  output.clear();
  char codepoint[7];
  for (auto ch : input) {
    ::tb_utf8_unicode_to_char(codepoint, ch);
    output.append(codepoint);
  }
}

} // namespace xxx
