// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <string_view>

namespace xxx {

struct im_vec2 {
  int x = 0;
  int y = 0;
};

struct im_color {
  std::uint64_t value = 0;
};

auto create_context() -> std::expected<void, std::string_view>;

void destroy_context();

void poll_events();

void new_frame();

void render();

} // namespace xxx
