// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx_internal.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace xxx::v2 {

[[nodiscard]] auto utf8_to_unicode(std::string_view input) -> std::span<std::uint32_t const>;

} // namespace xxx::v2
