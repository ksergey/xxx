// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <array>
#include <vector>

#include <termbox2.h>

#include "xxx2.h"

namespace xxx {

static_assert(std::is_same_v<uintattr_t, std::uint64_t>, "termbox2 invalid configuration");

// ------------------------------------
// common
// ------------------------------------

// clock type
using clock = std::chrono::steady_clock;

// ------------------------------------
// basic containers
// ------------------------------------
template <typename T>
using im_vector = std::vector<T>;

template <typename T, std::size_t N>
using im_array = std::array<T, N>;

// ------------------------------------
// context
// ------------------------------------
struct im_context {};

} // namespace xxx
