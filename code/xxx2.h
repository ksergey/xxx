// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace xxx {
inline namespace v2 {

struct im_vec2 {
  int x = 0;
  int y = 0;
};

struct im_rect {
  im_vec2 min;
  im_vec2 max;

  constexpr im_rect() = default;
  constexpr im_rect(im_vec2 const& mi, im_vec2 const& ma) noexcept : min(mi), max(ma) {}
  constexpr im_rect(int x1, int y1, int x2, int y2) noexcept : min(x1, y1), max(x2, y2) {}
};

enum class im_color : std::uint64_t {};

struct im_style {
  std::uint64_t fg;
  std::uint64_t bg;
};

enum class im_key : std::uint16_t {};

enum class im_char : std::uint32_t {};

void init();

void shutdown();

void poll_events(std::chrono::milliseconds timeout);

void new_frame();

void render();

} // namespace v2
} // namespace xxx
