// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace xxx {

// convert utf8 string to unicode
// WARNING: result valid until next call
// WARNING: non thread safe
[[nodiscard]] auto utf8_to_unicode(std::string_view input) -> std::span<std::uint32_t const>;

} // namespace xxx
