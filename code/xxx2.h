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

  [[nodiscard]] constexpr auto get_top_left() const noexcept -> im_vec2 {
    return min;
  }

  [[nodiscard]] constexpr auto get_top_right() const noexcept -> im_vec2 {
    return im_vec2{max.x, min.y};
  }

  [[nodiscard]] constexpr auto get_bottom_left() const noexcept -> im_vec2 {
    return im_vec2{min.x, max.y};
  }

  [[nodiscard]] constexpr auto get_bottom_right() const noexcept -> im_vec2 {
    return max;
  }

  [[nodiscard]] constexpr auto get_center() const noexcept -> im_vec2 {
    return im_vec2{(min.x + max.x) / 2, (min.y + max.y) / 2};
  }

  [[nodiscard]] constexpr auto get_size() const noexcept -> im_vec2 {
    return im_vec2{max.x - min.x, max.y - min.y};
  }

  [[nodiscard]] constexpr auto contains(im_vec2 const& p) const noexcept -> bool {
    return p.x >= min.x && p.y >= min.y && p.x < max.x && p.y < max.y;
  }

  [[nodiscard]] constexpr auto contains(im_rect const& r) const noexcept -> bool {
    return r.min.x >= min.x && r.min.y >= min.y && r.max.x <= max.x && r.max.y <= max.y;
  }

  constexpr void expand(int amount) noexcept {
    min.x -= amount;
    min.y -= amount;
    max.x += amount;
    max.y += amount;
  }

  constexpr void expand(im_vec2 const& amount) noexcept {
    min.x -= amount.x;
    min.y -= amount.y;
    max.x += amount.x;
    max.y += amount.y;
  }
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
