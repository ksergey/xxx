// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#pragma once

#include <iterator>
#include <string_view>
#include <type_traits>

#include <termbox.h>

namespace xxx::impl {
namespace utf8_detail {

// clang-format off
inline constexpr unsigned char length[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};
// clang-format on

inline constexpr unsigned char mask[6] = {0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

} // namespace utf8_detail

/// Return length of utf8 char in bytes.
/// @param[in] ch is first char of utf8 sequence.
constexpr std::size_t utf8_char_length(char ch) noexcept {
  return utf8_detail::length[static_cast<unsigned char>(ch)];
}

/// Check ch is utf8 symbol tail.
constexpr bool is_utf8_trail(char ch) noexcept {
  return (static_cast<unsigned char>(ch) >> 6) == 0x2;
}

/// Get utf8 string length.
constexpr std::size_t utf8_string_length(std::string_view str) noexcept {
  std::size_t result = 0;
  for (std::size_t i = 0; i < str.size();) {
    i += utf8_char_length(str[i]);
    ++result;
  }
  return result;
}

template<class OctectIterator>
constexpr std::uint32_t utf8_read_ucs(OctectIterator it) noexcept {
  auto const length = utf8_char_length(*it);
  auto const mask = utf8_detail::mask[length - 1];
  std::uint32_t result = *it & mask;
  for (std::size_t i = 1; i < length; ++i) {
    ++it;
    result = (result << 6) | (*it & 0x3f);
  }
  return result;
}

/// Increment iterator to start of next uft8 symbol.
template<class OctectIterator>
[[nodiscard]] inline OctectIterator utf8_next(OctectIterator it) noexcept {
  if (*it != '\0') [[likely]] {
    std::advance(it, utf8_char_length(*it));
  }
  return it;
}

/// Decrement iterator to start of prev uft8 symbol.
template<class OctectIterator>
[[nodiscard]] inline OctectIterator utf8_prev(OctectIterator it) noexcept {
  while (is_utf8_trail(*(--it))) {
  }
  return it;
}

template<class OctectIterator>
class utf8_iterator {
private:
  OctectIterator it_{};

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = std::uint32_t;
  using pointer = value_type*;
  using reference = value_type&;

  /// Construct empty iterator for late initialization.
  utf8_iterator() = default;

  /// Construct iterator from OctectIterator.
  explicit utf8_iterator(OctectIterator it) : it_{std::move(it)} {}

  /// Return base iterator.
  OctectIterator base() const {
    return it_;
  }

  /// Return utf8 symbol for iterator point to.
  value_type operator*() const noexcept {
    return utf8_read_ucs(it_);
  }

  /// Compare for equality.
  bool operator==(utf8_iterator const& rhs) const noexcept {
    return it_ == rhs.it_;
  }

  /// Compare for inequality.
  bool operator!=(utf8_iterator const& rhs) const noexcept {
    return it_ != rhs.it_;
  }

  /// Pre-increment.
  utf8_iterator& operator++() {
    std::advance(it_, utf8_char_length(*it_));
    return *this;
  }

  /// Post-increment.
  utf8_iterator operator++(int) {
    utf8_iterator temp(it_);
    std::advance(it_, utf8_char_length(*it_));
    return temp;
  }

  /// Pre-decrement.
  utf8_iterator& operator--() {
    it_ = utf8_prev(it_);
    return *this;
  }

  /// Post-decrement.
  utf8_iterator operator--(int) {
    utf8_iterator temp(*this);
    it_ = utf8_prev(it_);
    return temp;
  }
};

/// Make utf8_iterator from octet iterator.
template<class OctectIterator>
inline auto make_utf8_iterator(OctectIterator it) {
  return utf8_iterator<OctectIterator>{it};
}

/// Convert ucs charatecter into utf8 sequence.
template<class OctetIterator>
inline OctetIterator utf8_append(std::uint32_t ch, OctetIterator result) {
  if (ch < 0x80)
    *(result++) = static_cast<char>(ch);
  else if (ch < 0x800) {
    *(result++) = static_cast<char>((ch >> 6) | 0xc0);
    *(result++) = static_cast<char>((ch & 0x3f) | 0x80);
  } else if (ch < 0x10000) [[likely]] {
    *(result++) = static_cast<char>((ch >> 12) | 0xe0);
    *(result++) = static_cast<char>(((ch >> 6) & 0x3f) | 0x80);
    *(result++) = static_cast<char>((ch & 0x3f) | 0x80);
  } else {
    *(result++) = static_cast<char>((ch >> 18) | 0xf0);
    *(result++) = static_cast<char>(((ch >> 12) & 0x3f) | 0x80);
    *(result++) = static_cast<char>(((ch >> 6) & 0x3f) | 0x80);
    *(result++) = static_cast<char>((ch & 0x3f) | 0x80);
  }
  return result;
}

/// Convert sequence of uint32_t into utf8 string.
template<class OctectIterator, class U32Iterator>
inline OctectIterator u32_to_utf8(U32Iterator begin, U32Iterator end, OctectIterator result) {
  while (begin != end) {
    result = utf8_append(*(begin++), result);
  }
  return result;
}

} // namespace xxx::impl
