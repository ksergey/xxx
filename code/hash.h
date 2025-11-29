// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstring>
#include <ranges>
#include <span>
#include <type_traits>

namespace xxx {
namespace detail {

// murmur3 hash
[[nodiscard]] constexpr auto mm3_32(char const* data, std::size_t size, std::uint32_t seed) noexcept -> std::uint32_t {
  constexpr auto c1 = std::uint32_t(0xcc9e2d51);
  constexpr auto c2 = std::uint32_t(0x1b873593);

  auto const n_blocks = size / 4;
  auto const p_data = data;
  auto const p_tail = p_data + n_blocks * 4;

  auto hash = seed;

  for (auto const& block : std::span(p_data, p_tail) | std::views::chunk(4)) {
    auto chunk = std::uint32_t(block[0]);
    chunk |= std::uint32_t(block[1]) << 8;
    chunk |= std::uint32_t(block[2]) << 16;
    chunk |= std::uint32_t(block[3]) << 24;

    chunk *= c1;
    chunk = std::rotl(chunk, 15);
    chunk *= c2;

    hash ^= chunk;
    hash = std::rotl(hash, 13);
    hash = hash * 5 + std::uint32_t(0xe6546b64);
  }

  auto chunk = std::uint32_t(0);
  switch (size & 0x3) {
  case 3:
    chunk ^= std::uint32_t(p_tail[2]) << 16;
    [[fallthrough]];
  case 2:
    chunk ^= std::uint32_t(p_tail[1]) << 8;
    [[fallthrough]];
  case 1:
    chunk ^= std::uint32_t(p_tail[0]);
    chunk *= c1;
    chunk = std::rotl(chunk, 15);
    chunk *= c2;

    hash ^= chunk;
  default:
    break;
  }

  hash ^= static_cast<std::uint32_t>(size);
  // fmix32
  hash ^= hash >> 16;
  hash *= std::uint32_t(0x85ebca6b);
  hash ^= hash >> 13;
  hash *= std::uint32_t(0xc2b2ae35);
  hash ^= hash >> 16;

  return hash;
}

static_assert(mm3_32("1923cj32ASF}~", 13, 99913) == 2301554477);
static_assert(mm3_32("zo20u7Lfodi7", 12, 3318) == 2261267491);

[[nodiscard]] inline auto mm3_32(void const* data, std::size_t size, std::uint32_t seed) noexcept -> std::uint32_t {
  return mm3_32(static_cast<char const*>(data), size, seed);
}

} // namespace detail

// hash for numbers
template <typename T>
  requires std::is_integral_v<T>
[[nodiscard]] inline auto hash(T const& value, std::uint32_t seed) noexcept -> std::uint32_t {
  return detail::mm3_32(&value, sizeof(value), seed);
}

// hash for strings
[[nodiscard]] constexpr auto hash(std::string_view value, std::uint32_t seed) noexcept -> std::uint32_t {
  return detail::mm3_32(value.data(), value.size(), seed);
}

static_assert(hash("1923cj32ASF}~", 99913) == 2301554477);
static_assert(hash("zo20u7Lfodi7", 3318) == 2261267491);

} // namespace xxx
