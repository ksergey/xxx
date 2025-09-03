// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx_unicode.h"

#include <vector>

namespace xxx::v2 {

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

} // namespace xxx::v2
