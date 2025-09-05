// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <string_view>

namespace xxx {
inline namespace v2 {

struct im_vec2 {
  int x = 0;
  int y = 0;

  [[nodiscard]] friend constexpr auto operator+(im_vec2 const& a, im_vec2 const& b) noexcept -> im_vec2 {
    return im_vec2{a.x + b.x, a.y + b.y};
  }
  [[nodiscard]] friend constexpr auto operator-(im_vec2 const& a, im_vec2 const& b) noexcept -> im_vec2 {
    return im_vec2{a.x - b.x, a.y - b.y};
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

struct im_rect {
  int x;
  int y;
  int width;
  int height;

  constexpr im_rect() = default;
  constexpr im_rect(int x0, int y0, int width0, int height0) noexcept : x(x0), y(y0), width(width0), height(height0) {}
  constexpr im_rect(im_vec2 const& min, im_vec2 const& max) noexcept
      : x(min.x), y(min.y), width(max.x - min.x), height(max.y - min.y) {}

  constexpr auto operator<=>(im_rect const&) const noexcept = default;

  [[nodiscard]] constexpr auto top_left() const noexcept -> im_vec2 {
    return im_vec2{x, y};
  }
  [[nodiscard]] constexpr auto top_right() const noexcept -> im_vec2 {
    return im_vec2{x + width, y};
  }
  [[nodiscard]] constexpr auto bottom_left() const noexcept -> im_vec2 {
    return im_vec2{x, y + height};
  }
  [[nodiscard]] constexpr auto bottom_right() const noexcept -> im_vec2 {
    return im_vec2{x + width, y + height};
  }
  [[nodiscard]] constexpr auto get_pos() const noexcept -> im_vec2 {
    return im_vec2{x, y};
  }
  [[nodiscard]] constexpr auto get_center() const noexcept -> im_vec2 {
    return im_vec2{x + width / 2, y + height / 2};
  }
  [[nodiscard]] constexpr auto get_size() const noexcept -> im_vec2 {
    return im_vec2{width, height};
  }
  [[nodiscard]] constexpr auto contains(im_vec2 const& p) const noexcept -> bool {
    auto const min = this->top_left();
    auto const max = this->bottom_right();
    return p.x >= min.x && p.y >= min.y && p.x < max.x && p.y < max.y;
  }
  [[nodiscard]] constexpr auto contains(im_rect const& r) const noexcept -> bool {
    auto const min = this->top_left();
    auto const max = this->bottom_right();
    auto const r_min = r.top_left();
    auto const r_max = r.bottom_right();
    return r_min.x >= min.x && r_min.y >= min.y && r_max.x <= max.x && r_max.y <= max.y;
  }
  [[nodiscard]] constexpr auto empty_area() const noexcept -> bool {
    return width == 0 || height == 0;
  }
  [[nodiscard]] constexpr auto adjusted(int value) const noexcept -> im_rect {
    return this->adjusted(value, value, value, value);
  }
  [[nodiscard]] constexpr auto adjusted(int left, int top, int right, int bottom) const noexcept -> im_rect {
    return im_rect(x - left, y - top, width + left + right, height + top + bottom);
  }
  [[nodiscard]] constexpr auto adjusted_left(int value) const noexcept -> im_rect {
    return this->adjusted(value, 0, 0, 0);
  }
  [[nodiscard]] constexpr auto adjusted_top(int value) const noexcept -> im_rect {
    return this->adjusted(0, value, 0, 0);
  }
  [[nodiscard]] constexpr auto adjusted_right(int value) const noexcept -> im_rect {
    return this->adjusted(0, 0, value, 0);
  }
  [[nodiscard]] constexpr auto adjusted_bottom(int value) const noexcept -> im_rect {
    return this->adjusted(0, 0, 0, value);
  }
};

enum class im_color : std::uint32_t {};

constexpr auto get_color(float r, float g, float b) noexcept -> im_color {
  return im_color((std::uint32_t(std::uint8_t(r * 255)) << 16) | std::uint32_t(std::uint8_t(g * 255)) << 8 |
                  std::uint32_t(std::uint8_t(b * 255)));
}

constexpr auto get_color(std::uint32_t value) noexcept -> im_color {
  return im_color(value);
}

inline namespace literals {

constexpr auto operator""_c(unsigned long long int value) noexcept -> im_color {
  return get_color(static_cast<std::uint32_t>(value));
}

} // namespace literals

// default color as terminal configured
[[nodiscard]] auto get_default_color() noexcept -> im_color;

// drawing style
struct im_style {
  std::uint64_t fg;
  std::uint64_t bg;

  constexpr im_style() = default;

  constexpr im_style(im_color fg0, im_color bg0 = get_default_color())
      : fg(static_cast<std::uint64_t>(fg0)), bg(static_cast<std::uint64_t>(bg0)) {}
};

enum class im_key_id { tab, enter, esc, quit, last };

enum class im_mouse_button_id { left, middle, right, last };

// ----------------------------------------------------------------------------
// input utilities
// ----------------------------------------------------------------------------
[[nodiscard]] auto is_key_pressed(im_key_id id) -> bool;
[[nodiscard]] auto is_mouse_pressed(im_mouse_button_id id) -> bool;
[[nodiscard]] auto is_mouse_hovering_rect(im_rect const& rect) -> bool;
[[nodiscard]] auto get_mouse_pos() -> im_vec2;

// ----------------------------------------------------------------------------
// window utilities
// ----------------------------------------------------------------------------
[[nodiscard]] auto get_window_size() -> im_vec2;

// ----------------------------------------------------------------------------
// layout functions
// ----------------------------------------------------------------------------
void layout_set_min_height(int height);
void layout_row_begin(int height, std::size_t columns);
void layout_row_push(float ratio_or_width);
void layout_row_end();

// return total space allocated for a current layout
[[nodiscard]] auto layout_get_space_bounds() -> im_rect;

// TODO:
void layout_scrollable_begin(im_vec2 const& size, im_vec2 const& offset);
void layout_scrollable_end();

// ----------------------------------------------------------------------------
// library main
// ----------------------------------------------------------------------------
void init();
void shutdown();

// poll terminal events
// needs to call for each process loop iteration
void poll_terminal_events(std::chrono::milliseconds timeout);

// start drawing new frame
void new_frame();
// redner frame
void render();

// ----------------------------------------------------------------------------
// widgets
// ----------------------------------------------------------------------------

// widget: label
void label(std::string_view text);

// widget: label
template <typename... Ts>
void label(std::format_string<Ts...> fmt, Ts&&... args) {
  char buffer[64];
  auto const result = std::format_to_n(buffer, sizeof(buffer), fmt, std::forward<Ts>(args)...);
  label(std::string_view(buffer, result.size));
}

void debug();
void debug_rect();

} // namespace v2
} // namespace xxx
