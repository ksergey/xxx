// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace xxx {

struct im_vec2 {
  int x = 0;
  int y = 0;
};

struct im_color {
  std::uint64_t value = 0;
};

enum class im_key : std::uint16_t {};

void create_context();

void destroy_context();

void poll_events(std::chrono::milliseconds timeout);

void new_frame();

void render();

} // namespace xxx
