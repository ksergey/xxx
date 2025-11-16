// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <span>
#include <string_view>

namespace xxx {

// Returns a view of the substring [pos, pos + rlen), where rlen is the smaller of count and size() - pos.
template <typename CharT, typename Traits>
[[nodiscard]] constexpr auto substr(std::basic_string_view<CharT, Traits> str, std::size_t pos = 0,
    std::size_t count = std::basic_string_view<CharT, Traits>::npos) noexcept -> std::basic_string_view<CharT, Traits> {
  return str.substr(pos, count);
}

// Returns a view of the subspan [pos, pos + rlen), where rlen is the smaller of count and size() - pos.
template <typename T>
[[nodiscard]] constexpr auto substr(
    std::span<T> str, std::size_t pos = 0, std::size_t count = std::dynamic_extent) noexcept -> std::span<T> {
  if (pos > str.size()) {
    return {};
  }
  return str.subspan(pos, std::min(count, str.size() - pos));
}

} // namespace xxx
