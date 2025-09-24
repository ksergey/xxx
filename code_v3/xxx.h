// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <string_view>

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
  im_vec2 min = im_vec2(0, 0);   // top-left (included)
  im_vec2 max = im_vec2(-1, -1); // bottom-right (included)

  constexpr im_rect() = default;

  constexpr im_rect(im_vec2 const min0, im_vec2 const& max0) noexcept : min(min0), max(max0) {}

  constexpr im_rect(int x0, int y0, int x1, int y1) noexcept : min{x0, y0}, max{x1, y1} {}

  [[nodiscard]] constexpr operator bool() const noexcept {
    return !this->empty();
  }
  [[nodiscard]] constexpr auto null() const noexcept -> bool {
    return (min.x - 1 == max.x) && (min.y - 1 == max.y);
  }
  [[nodiscard]] constexpr auto empty() const noexcept -> bool {
    return (min.x > max.x) || (min.y > max.y);
  }
  [[nodiscard]] constexpr auto valid() const noexcept -> bool {
    return (min.x <= max.x) && (min.y <= max.y);
  }
  [[nodiscard]] constexpr auto center() const noexcept -> im_vec2 {
    return im_vec2((min.x + max.x) / 2, (min.y + max.y) / 2);
  }
  [[nodiscard]] constexpr auto size() const noexcept -> im_vec2 {
    return im_vec2(this->width(), this->height());
  }
  constexpr void set_size(im_vec2 const& s) noexcept {
    max = min + s - im_vec2(1, 1);
  }
  [[nodiscard]] constexpr auto width() const noexcept -> int {
    return max.x - min.x + 1;
  }
  constexpr void set_width(int v) noexcept {
    max.x = min.x + v - 1;
  }
  [[nodiscard]] constexpr auto height() const noexcept -> int {
    return max.y - min.y + 1;
  }
  constexpr void set_height(int v) noexcept {
    max.y = min.y + v - 1;
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
    if (this->null() || r.null()) {
      return im_rect();
    }
    auto const& r_min = r.min;
    auto const& r_max = r.max;
    return im_rect(im_vec2(std::clamp(min.x, r_min.x, r_max.x), std::clamp(min.y, r_min.y, r_max.y)),
        im_vec2(std::clamp(max.x, r_min.x, r_max.x), std::clamp(max.y, r_min.y, r_max.y)));
  }
  [[nodiscard]] constexpr auto translate(im_vec2 const& t) const noexcept -> im_rect {
    return im_rect(min + t, max + t);
  }
  [[nodiscard]] constexpr auto crop(int left, int top, int right, int bottom) const noexcept -> im_rect {
    return im_rect(min.x + left, min.y + top, max.x - right, max.y - bottom);
  }
  [[nodiscard]] constexpr auto crop_left(int value) const noexcept -> im_rect {
    return this->crop(value, 0, 0, 0);
  }
  [[nodiscard]] constexpr auto crop_top(int value) const noexcept -> im_rect {
    return this->crop(0, value, 0, 0);
  }
  [[nodiscard]] constexpr auto crop_right(int value) const noexcept -> im_rect {
    return this->crop(0, 0, value, 0);
  }
  [[nodiscard]] constexpr auto crop_bottom(int value) const noexcept -> im_rect {
    return this->crop(0, 0, 0, value);
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
enum class im_key_id { tab, enter, esc, space, quit, last };

// mouse button id
enum class im_mouse_button_id { left, middle, right, last };

} // namespace literals

// helper: color id
enum class im_color_id { fg, bg, btn_fg, btn_fg_act, btn_fg_hov, btn_bg, btn_bg_act, btn_bg_hov, last };

constexpr auto init_flags_mouse = int(1 << 0);

// init library
void init(int flags = 0);

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

// set default color
void set_default_color(im_color_id id, im_color color);

// save current color and set new
void push_color(im_color_id id, im_color color);

// pop color state
void pop_color(std::size_t cnt = 1);

// push id state
void push_id(std::string_view value);

// overload
void push_id(int value);

// pop id state
void pop_id();

// widget: label
void label(std::string_view text);

// widget: button
auto button(std::string_view text) -> bool;

} // namespace xxx
