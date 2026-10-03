// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "unicode.h"

#include <algorithm>

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

auto char_width(std::uint32_t ch) noexcept -> int {
  if (ch < 0x80) [[likely]] {
    return 1;
  }
  return ::tb_wcwidth(ch) >= 2 ? 2 : 1;
}

auto text_width(std::span<std::uint32_t const> text) noexcept -> int {
  auto width = 0;
  for (auto const ch : text) {
    width += char_width(ch);
  }
  return width;
}

auto slice_columns(std::span<std::uint32_t const> text, int skip, int max_width) noexcept -> text_slice {
  auto result = text_slice();
  if (max_width <= 0) {
    return result;
  }
  skip = std::max(skip, 0);

  auto const size = text.size();
  auto i = std::size_t(0);
  auto column = 0;

  // drop leading columns
  while (i < size && column + char_width(text[i]) <= skip) {
    column += char_width(text[i++]);
  }
  auto used = 0;
  if (i < size && column < skip) {
    // wide char cut by left edge
    column += char_width(text[i++]);
    result.pad_left = 1;
    used = 1;
  }

  // take columns which fit
  auto const first = i;
  while (i < size && used + char_width(text[i]) <= max_width) {
    used += char_width(text[i++]);
  }
  result.text = text.subspan(first, i - first);

  if (i < size && used < max_width) {
    // wide char cut by right edge
    result.pad_right = 1;
  }
  return result;
}

} // namespace xxx
