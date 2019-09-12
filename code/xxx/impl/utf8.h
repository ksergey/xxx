// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#ifndef KSERGEY_utf8_300819122133
#define KSERGEY_utf8_300819122133

#include <iterator>
#include <string_view>
#include <type_traits>

#include <termbox.h>

#include "compiler.h"

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

}  // namespace utf8_detail

/// Return length of utf8 char in bytes.
/// @param[in] ch is first char of utf8 sequence.
constexpr std::size_t utf8_char_length(char ch) noexcept { return utf8_detail::length[static_cast<unsigned char>(ch)]; }

/// Check ch is utf8 symbol tail.
constexpr bool is_utf8_trail(char ch) noexcept { return (static_cast<unsigned char>(ch) >> 6) == 0x2; }

/// Get utf8 string length.
constexpr std::size_t utf8_string_length(std::string_view str) noexcept {
  std::size_t result = 0;
  for (std::size_t i = 0; i < str.size();) {
    i += utf8_char_length(str[i]);
    ++result;
  }
  return result;
}

/// Remove prefix from utf8 string.
constexpr std::string_view utf8_remove_prefix(std::string_view str, std::size_t count) noexcept {
  std::size_t count_to_remove{0};

  for (auto it = str.begin(); it != str.end() && count > 0; count--) {
    auto length = utf8_char_length(*it);
    it += length;
    count_to_remove += length;
  }

  str.remove_prefix(count_to_remove);
  return str;
}

namespace test {

inline constexpr auto str = std::string_view{"★"};

static_assert(str.size() == 3);
static_assert(utf8_string_length(str) == 1);
static_assert(!is_utf8_trail(str[0]));
static_assert(is_utf8_trail(str[1]));
static_assert(is_utf8_trail(str[2]));

}  // namespace test

template <class OctectIterator>
XXX_ALWAYS_INLINE std::uint32_t utf8_next(OctectIterator& it) noexcept {
  if (XXX_UNLIKELY(*it == '\0')) {
    return 0;
  }

  auto const length = utf8_char_length(*it);
  auto const mask = utf8_detail::mask[length - 1];
  std::uint32_t result = *it & mask;
  for (std::size_t i = 1; i < length; ++i) {
    ++it;
    result = (result << 6) | (*it & 0x3f);
  }
  return result;
}

template <class OctectIterator>
XXX_ALWAYS_INLINE std::uint32_t utf8_prior(OctectIterator& it) noexcept {
  while (is_utf8_trail(*(--it))) {
  }
  OctectIterator temp{it};
  return utf8_next(temp);
}

template <class OctectIterator>
class utf8_iterator : public std::iterator<std::bidirectional_iterator_tag, std::uint32_t> {
 private:
  OctectIterator it_{};

 public:
  /// Construct empty iterator for late initialization.
  utf8_iterator() = default;

  /// Construct iterator from OctectIterator.
  explicit utf8_iterator(OctectIterator it) : it_{std::move(it)} {}

  /// Return base iterator.
  XXX_ALWAYS_INLINE OctectIterator base() const { return it_; }

  /// Return utf8 symbol for iterator point to.
  XXX_ALWAYS_INLINE std::uint32_t operator*() const noexcept {
    OctectIterator temp{it_};
    return utf8_next(temp);
  }

  /// Compare for equality.
  XXX_ALWAYS_INLINE bool operator==(utf8_iterator const& rhs) const noexcept { return it_ == rhs.it_; }

  /// Compare for inequality.
  XXX_ALWAYS_INLINE bool operator!=(utf8_iterator const& rhs) const noexcept { return it_ != rhs.it_; }

  /// Pre-increment.
  XXX_ALWAYS_INLINE utf8_iterator& operator++() {
    std::advance(it_, utf8_char_length(*it_));
    return *this;
  }

  /// Post-increment.
  XXX_ALWAYS_INLINE utf8_iterator operator++(int) {
    utf8_iterator temp{it_};
    std::advance(it_, utf8_char_length(*it_));
    return temp;
  }

  /// Pre-decrement.
  XXX_ALWAYS_INLINE utf8_iterator& operator--() {
    utf8_prior(it_);
    return *this;
  }

  /// Post-decrement.
  XXX_ALWAYS_INLINE utf8_iterator operator--(int) {
    utf8_iterator temp{*this};
    utf8_prior(it_);
    return temp;
  }
};

/// Make utf8_iterator from octet iterator.
template <class OctectIterator>
XXX_ALWAYS_INLINE auto make_utf8_iterator(OctectIterator it) {
  return utf8_iterator<OctectIterator>{it};
}

template <class OctetIterator>
XXX_ALWAYS_INLINE OctetIterator utf8_append(std::uint32_t ch, OctetIterator result) {
  if (ch < 0x80)
    *(result++) = static_cast<char>(ch);
  else if (ch < 0x800) {
    *(result++) = static_cast<char>((ch >> 6) | 0xc0);
    *(result++) = static_cast<char>((ch & 0x3f) | 0x80);
  } else if (XXX_LIKELY(ch < 0x10000)) {
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

template <class OctectIterator, class U32Iterator>
XXX_ALWAYS_INLINE OctectIterator utf32_to_utf8(U32Iterator begin, U32Iterator end, OctectIterator result) {
  while (begin != end) {
    result = utf8_append(*(begin++), result);
  }
  return result;
}

}  // namespace xxx::impl

#endif /* KSERGEY_utf8_300819122133 */
