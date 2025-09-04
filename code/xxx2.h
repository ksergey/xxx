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
  [[nodiscard]] constexpr auto expanded(int amount) const noexcept -> im_rect {
    return this->expanded(im_vec2{amount, amount});
  }
  [[nodiscard]] constexpr auto expanded(im_vec2 const& amount) const noexcept -> im_rect {
    return im_rect(x - amount.x, y - amount.y, width + amount.x * 2, height + amount.y * 2);
  }
  [[nodiscard]] constexpr auto translated(im_vec2 const& distance) const noexcept -> im_rect {
    return im_rect(x + distance.x, y + distance.y, width, height);
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

auto get_default_color() noexcept -> im_color;

struct im_style {
  std::uint64_t fg;
  std::uint64_t bg;
};

constexpr auto make_style(im_color fg, im_color bg = get_default_color()) noexcept {
  return im_style{.fg = static_cast<std::uint64_t>(fg), .bg = static_cast<std::uint64_t>(bg)};
}

enum class im_key_id { tab, enter, esc, quit, last };

enum class im_mouse_button_id { left, middle, right, last };

// input utilities
[[nodiscard]] auto is_key_pressed(im_key_id id) -> bool;
[[nodiscard]] auto is_mouse_pressed(im_mouse_button_id id) -> bool;
[[nodiscard]] auto is_mouse_hovering_rect(im_rect const& rect) -> bool;
[[nodiscard]] auto get_mouse_pos() -> im_vec2;

// window utilities
auto get_window_size() -> im_vec2;

void init();
void shutdown();

void poll_terminal_events(std::chrono::milliseconds timeout);

void new_frame();
void render();

void label(std::string_view text);

template <typename... Ts>
void label(std::format_string<Ts...> fmt, Ts&&... args) {
  char buffer[64];
  auto const result = std::format_to_n(buffer, sizeof(buffer), fmt, std::forward<Ts>(args)...);
  label(std::string_view(buffer, result.size));
}

void show_debug();

} // namespace v2
} // namespace xxx
