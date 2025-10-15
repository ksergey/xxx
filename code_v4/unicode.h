// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace xxx {

// convert utf8 string to unicode
// WARNING: result valid until next call
[[nodiscard]] auto utf8_to_unicode(std::string_view input) -> std::span<std::uint32_t const>;

// convert utf8 string to unicode
void utf8_to_unicode(std::string_view input, std::vector<std::uint32_t>& output);

// convert unicode string into utf8 string
// WARNING: result valid until next call
[[nodiscard]] auto unicode_to_utf8(std::span<std::uint32_t const> input) -> std::string_view;

// convert unicode string into utf8 string
void unicode_to_utf8(std::span<std::uint32_t const> input, std::string& output);

} // namespace xxx
