// ------------------------------------------------------------
// Copyright 2022-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#include "utf8.h"

namespace xxx::impl {

inline constexpr auto str = std::string_view{"★"};

static_assert(str.size() == 3);
static_assert(utf8_string_length(str) == 1);
static_assert(!is_utf8_trail(str[0]));
static_assert(is_utf8_trail(str[1]));
static_assert(is_utf8_trail(str[2]));

} // namespace xxx::impl
