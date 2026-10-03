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

// number of terminal cells occupied by codepoint: 2 for wide (CJK, emoji), 1 otherwise
// matches termbox2 rendering, which draws zero-width and non-printable codepoints in one cell
[[nodiscard]] auto char_width(std::uint32_t ch) noexcept -> int;

// number of terminal cells occupied by text
[[nodiscard]] auto text_width(std::span<std::uint32_t const> text) noexcept -> int;

// part of text visible in a window of terminal columns
// wide char cut by the window edge is replaced with a single space (pad_left / pad_right)
struct text_slice {
  std::span<std::uint32_t const> text;
  int pad_left = 0;
  int pad_right = 0;

  [[nodiscard]] auto width() const noexcept -> int {
    return pad_left + text_width(text) + pad_right;
  }
  [[nodiscard]] auto empty() const noexcept -> bool {
    return text.empty() && pad_left == 0 && pad_right == 0;
  }
};

// slice text to columns [skip, skip + max_width)
[[nodiscard]] auto slice_columns(std::span<std::uint32_t const> text, int skip, int max_width) noexcept -> text_slice;

} // namespace xxx
