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
  im_vec2 min; // upper-left
  im_vec2 max; // bottom-right

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
  [[nodiscard]] constexpr auto get_width() const noexcept -> int {
    return get_size().x;
  }
  [[nodiscard]] constexpr auto get_height() const noexcept -> int {
    return get_size().y;
  }
  [[nodiscard]] constexpr auto contains(im_vec2 const& p) const noexcept -> bool {
    return p.x >= min.x && p.y >= min.y && p.x < max.x && p.y < max.y;
  }
  [[nodiscard]] constexpr auto contains(im_rect const& r) const noexcept -> bool {
    return r.min.x >= min.x && r.min.y >= min.y && r.max.x <= max.x && r.max.y <= max.y;
  }
  [[nodiscard]] constexpr auto empty() const noexcept -> bool {
    return min.x == max.x || min.y == max.y;
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

void layout_row_begin(int min_height, std::size_t columns);
void layout_row_push(float width_or_ratio);
void layout_row_end();

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
