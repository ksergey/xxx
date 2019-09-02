// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#ifndef KSERGEY_utf8_300819122133
#define KSERGEY_utf8_300819122133

#include <string_view>

#include <termbox.h>

#include "compiler.h"

namespace xxx::impl {

XXX_ALWAYS_INLINE std::size_t utf8_string_length(std::string_view str) noexcept {
  std::size_t result = 0;
  for (std::size_t i = 0; i < str.size();) {
    i += tb_utf8_char_length(str[i]);
    ++result;
  }
  return result;
}

}  // namespace xxx::impl

#endif /* KSERGEY_utf8_300819122133 */
