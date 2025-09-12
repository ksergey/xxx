// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

namespace xxx {

// helper: 2d vector
struct im_vec2 {
  int x = 0;
  int y = 0;

  constexpr im_vec2() = default;
  constexpr im_vec2(int x0, int y0) noexcept : x(x0), y(y0) {}

  [[nodiscard]] friend constexpr auto operator+(im_vec2 const& a, im_vec2 const& b) noexcept -> im_vec2 {
    return im_vec2(a.x + b.x, a.y + b.y);
  }
  [[nodiscard]] friend constexpr auto operator-(im_vec2 const& a, im_vec2 const& b) noexcept -> im_vec2 {
    return im_vec2(a.x - b.x, a.y - b.y);
  }
  constexpr auto operator+=(im_vec2 const& a) noexcept -> im_vec2& {
    x += a.x;
    y += a.y;
    return *this;
  }
  constexpr auto operator-=(im_vec2 const& a) noexcept -> im_vec2& {
    x -= a.x;
    y -= a.y;
    return *this;
  }

  constexpr auto operator<=>(im_vec2 const&) const noexcept = default;
};

// helper: aligned bounding-box
struct im_rect {
  im_vec2 min = im_vec2(0, 0); // top-left (included)
  im_vec2 max = im_vec2(0, 0); // bottom-right (included)

  constexpr im_rect() = default;

  constexpr im_rect(im_vec2 const min0, im_vec2 const& max0) noexcept : min(min0), max(max0) {}

  constexpr im_rect(int x0, int y0, int x1, int y1) noexcept : min{x0, y0}, max{x1, y1} {}

  [[nodiscard]] constexpr auto valid() const noexcept -> bool {
    return (min.x <= max.x) && (min.y <= max.y);
  }
  [[nodiscard]] constexpr auto empty() const noexcept -> bool {
    return (min.x > max.x) || (min.y > max.y);
  }
  [[nodiscard]] constexpr auto center() const noexcept -> im_vec2 {
    return im_vec2((min.x + max.x) / 2, (min.y + max.y) / 2);
  }
  [[nodiscard]] constexpr auto size() const noexcept -> im_vec2 {
    return im_vec2(this->width(), this->height());
  }
  [[nodiscard]] constexpr auto width() const noexcept -> int {
    return max.x - min.x + 1;
  }
  [[nodiscard]] constexpr auto height() const noexcept -> int {
    return max.y - min.y + 1;
  }
  [[nodiscard]] constexpr auto top_left() const noexcept -> im_vec2 {
    return min;
  }
  [[nodiscard]] constexpr auto bottom_right() const noexcept -> im_vec2 {
    return max;
  }
  [[nodiscard]] constexpr auto top_right() const noexcept -> im_vec2 {
    return im_vec2(max.x, min.y);
  }
  [[nodiscard]] constexpr auto bottom_left() const noexcept -> im_vec2 {
    return im_vec2(min.x, max.y);
  }
  [[nodiscard]] constexpr auto contains(im_vec2 const& p) const noexcept -> bool {
    return p.x >= min.x && p.y >= min.y && p.x <= max.x && p.y <= max.y;
  }
  [[nodiscard]] constexpr auto contains(im_rect const& r) const noexcept -> bool {
    return r.min.x >= min.x && r.min.y >= min.y && r.max.x <= max.x && r.max.y <= max.y;
  }
  [[nodiscard]] constexpr auto intersection(im_rect const& r) const noexcept -> im_rect {
    auto const& r_min = r.min;
    auto const& r_max = r.max;
    return im_rect(im_vec2(std::clamp(min.x, r_min.x, r_max.x), std::clamp(min.y, r_min.y, r_max.y)),
        im_vec2(std::clamp(max.x, r_min.x, r_max.x), std::clamp(max.y, r_min.y, r_max.y)));
  }
  [[nodiscard]] constexpr auto translate(im_vec2 const& t) const noexcept -> im_rect {
    return im_rect(min + t, max + t);
  }

  constexpr auto operator<=>(im_rect const&) const noexcept = default;
};

// helper: internal color representation
enum class im_color : std::uint32_t {};

// get default color as terminal configured
[[nodiscard]] auto get_default_color() noexcept -> im_color;

// convert from rgb to color
[[nodiscard]] constexpr auto make_color(float r, float g, float b) noexcept -> im_color {
  return im_color((std::uint32_t(std::uint8_t(r * 255)) << 16) | std::uint32_t(std::uint8_t(g * 255)) << 8 |
                  std::uint32_t(std::uint8_t(b * 255)));
}

// convert from 0xRRGGBB to color
[[nodiscard]] constexpr auto make_color(std::uint32_t value) noexcept -> im_color {
  return im_color(value);
}

inline namespace literals {

[[nodiscard]] constexpr auto operator""_c(unsigned long long int value) noexcept -> im_color {
  return make_color(static_cast<std::uint32_t>(value));
}

// keyboard key id
enum class im_key_id { tab, enter, esc, quit, last };

// mouse button id
enum class im_mouse_button_id { left, middle, right, last };

} // namespace literals

// default color as terminal configured
[[nodiscard]] auto get_default_color() noexcept -> im_color;

// init library
void init();

// shutdown library
void shutdown();

// update internal state
void update(std::chrono::milliseconds timeout);

// start drawing new frame
void new_frame();

// render frame
void render();

// check key pressed
[[nodiscard]] auto is_key_pressed(im_key_id id) -> bool;

// check mouse button pressed
[[nodiscard]] auto is_mouse_pressed(im_mouse_button_id id) -> bool;

// get mouse position
[[nodiscard]] auto get_mouse_pos() -> im_vec2;

// get window size
[[nodiscard]] auto get_window_size() -> im_vec2;

// get window bounds
[[nodiscard]] auto get_window_bounds() -> im_rect;

void debug();

} // namespace xxx
