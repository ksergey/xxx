// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <ranges>
#include <span>
#include <type_traits>
#include <vector>

#include <termbox2.h>

#include "im_stack.h"
#include "string_utils.h"
#include "xxx.h"

#if 0
#include <print>
namespace xxx {

template <typename... Ts>
void debug(std::format_string<Ts...> fmt, Ts&&... args) {
  FILE* file = ::fopen("debug.txt", "a");
  if (!file) {
    return;
  }
  std::print(file, fmt, std::forward<Ts>(args)...);
  std::print(file, "\n");
  ::fclose(file);
}

} // namespace xxx
#endif

namespace xxx {

struct im_style {
  std::uint64_t fg = 0;
  std::uint64_t bg = 0;

  constexpr im_style() = default;
  constexpr im_style(im_color color_fg, im_color color_bg = im_color()) noexcept : fg(color_fg), bg(color_bg) {}

  [[nodiscard]] constexpr auto with_underline() const noexcept -> im_style {
    return im_style(im_color(fg | TB_UNDERLINE), im_color(bg));
  }
  [[nodiscard]] constexpr auto with_reverse() const noexcept -> im_style {
    return im_style(im_color(fg | TB_REVERSE), im_color(bg));
  }
  [[nodiscard]] constexpr auto with_blink() const noexcept -> im_style {
    return im_style(im_color(fg | TB_BLINK), im_color(bg));
  }
};

struct im_cell {
  uint32_t ch;
  im_style style;
};

enum class im_halign { left, center, right };
enum class im_valign { top, center, bottom };

class im_renderer {
private:
  static constexpr auto render_cmd_text_max_size = std::size_t(32);
  static constexpr auto border_style = std::to_array<std::uint32_t>({L'╭', L'╮', L'╰', L'╯', L'│', L'─'});

  enum class render_cmd_type { none, fill_rect, draw_rect, draw_text, draw_surface };

  struct render_cmd_none {};

  struct render_cmd_fill_rect_data {
    im_rect rect;
    std::uint32_t ch;
  };

  struct render_cmd_draw_rect_data {
    im_rect rect;
  };

  struct render_cmd_draw_text_data {
    im_vec2 pos;
    // non-owning value
    // should be alive until start_new_frame call
    std::span<std::uint32_t const> text;
  };

  struct render_cmd_draw_surface_data {
    im_rect src_rect;
    im_rect rect;
    std::span<im_cell const> data;
  };

  struct render_cmd {
    render_cmd_type type = render_cmd_type::none;
    im_style style;
    union {
      render_cmd_none none = {};
      render_cmd_fill_rect_data fill_rect_data;
      render_cmd_draw_rect_data draw_rect_data;
      render_cmd_draw_text_data draw_text_data;
      render_cmd_draw_surface_data draw_surface_data;
    };
  };
  static_assert(std::is_trivially_copyable_v<render_cmd>);

  im_stack<im_rect> clip_rect_stack_ = im_stack<im_rect>(32);
  im_vec2 viewport_offset_;
  im_rect clip_rect_;
  std::vector<render_cmd> commands_;

public:
  im_renderer(im_renderer const&) = delete;
  im_renderer& operator=(im_renderer const&) = delete;
  im_renderer() = default;

  [[nodiscard]] auto viewport_offset() const noexcept -> im_vec2 const& {
    return viewport_offset_;
  }

  void set_viewport_offset(im_vec2 const& value) noexcept {
    viewport_offset_ = value;
  }

  [[nodiscard]] auto is_visible(im_rect const& rect) const noexcept {
    return !clip_rect_.intersection(rect.translate(-viewport_offset_)).empty();
  }

  [[nodiscard]] auto is_visible(im_vec2 const& point) const noexcept {
    return clip_rect_.contains(point - viewport_offset_);
  }

  [[nodiscard]] auto clip_rect() const noexcept -> im_rect const& {
    return clip_rect_;
  }

  void push_clip_rect(im_rect const& rect, bool clip_to_parent = true) {
    clip_rect_stack_.push_back(clip_rect_);
    clip_rect_ = clip_to_parent ? clip_rect_.intersection(rect) : rect;
  }

  void pop_clip_rect() {
    if (clip_rect_stack_.empty()) [[unlikely]] {
      return;
    }
    clip_rect_ = clip_rect_stack_.back();
    clip_rect_stack_.pop_back();
  }

  void set_clear_color(im_style const& style) noexcept {
    ::tb_set_clear_attrs(style.fg, style.bg);
  }

  /// Start drawing new frame
  void start_new_frame(im_rect const& clip_rect);

  /// Render frame
  void render();

  /// Append command to fill rect
  void cmd_fill_rect(im_rect const& rect, std::uint32_t ch, im_style const& style) {
    auto const a_rect = this->adjust(rect);
    auto const c_rect = clip_rect_.intersection(a_rect);
    if (!c_rect) [[unlikely]] {
      return;
    }

    this->append_cmd_fill_rect(c_rect, ch, style);
  }

  /// Append command to draw rect
  void cmd_draw_rect(im_rect const& rect, im_style const& style) {
    auto const a_rect = this->adjust(rect);
    auto const c_rect = clip_rect_.intersection(a_rect);
    if (!c_rect) {
      return;
    }
    if (c_rect == a_rect) {
      // nothing to clip
      return this->append_cmd_draw_rect(a_rect, style);
    }

    auto const top_left = a_rect.top_left();
    auto const top_right = a_rect.top_right();
    auto const bottom_left = a_rect.bottom_left();
    auto const bottom_right = a_rect.bottom_right();

    auto const hline_min_x = std::clamp<int>(top_left.x + 1, clip_rect_.min.x, clip_rect_.max.x);
    auto const hline_max_x = std::clamp<int>(top_right.x - 1, clip_rect_.min.x, clip_rect_.max.x);

    auto const vline_min_y = std::clamp<int>(top_left.y + 1, clip_rect_.min.y, clip_rect_.max.y);
    auto const vline_max_y = std::clamp<int>(bottom_left.y - 1, clip_rect_.min.y, clip_rect_.max.y);

    if (top_left.y >= clip_rect_.min.y) {
      if (top_left.x >= clip_rect_.min.x) {
        this->append_cmd_fill_rect(im_rect(top_left, top_left), border_style[0], style);
      }
      if (top_right.x <= clip_rect_.max.x) {
        this->append_cmd_fill_rect(im_rect(top_right, top_right), border_style[1], style);
      }
      if (hline_min_x < hline_max_x) {
        this->append_cmd_fill_rect(
            im_rect(hline_min_x, a_rect.min.y, hline_max_x, a_rect.min.y), border_style[5], style);
      }
    }

    if (bottom_left.y <= clip_rect_.max.y) {
      if (bottom_left.x >= clip_rect_.min.x) {
        this->append_cmd_fill_rect(im_rect(bottom_left, bottom_left), border_style[2], style);
      }
      if (bottom_right.x <= clip_rect_.max.x) {
        this->append_cmd_fill_rect(im_rect(bottom_right, bottom_right), border_style[3], style);
      }
      if (hline_min_x < hline_max_x) {
        this->append_cmd_fill_rect(
            im_rect(hline_min_x, a_rect.max.y, hline_max_x, a_rect.max.y), border_style[5], style);
      }
    }

    if (vline_min_y <= vline_max_y) {
      if (top_left.x >= clip_rect_.min.x) {
        this->append_cmd_fill_rect(
            im_rect(a_rect.min.x, vline_min_y, a_rect.min.x, vline_max_y), border_style[4], style);
      }
      if (top_right.x <= clip_rect_.max.x) {
        this->append_cmd_fill_rect(
            im_rect(a_rect.max.x, vline_min_y, a_rect.max.x, vline_max_y), border_style[4], style);
      }
    }
  }

  /// Append command to draw text at position
  /// \warning \c text must exists until next im_renderer::start_new_frame(...) call
  void cmd_draw_text_at(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) {
    if (text.empty()) {
      return;
    }

    auto const a_rect = this->adjust(im_rect(pos, pos + im_vec2(text.size() - 1, 0)));
    auto const c_rect = clip_rect_.intersection(a_rect);
    if (!c_rect) {
      return;
    }

    if (c_rect.min.x > a_rect.min.x) {
      text = substr(text, c_rect.min.x - a_rect.min.x);
    }
    if (c_rect.max.x < a_rect.max.x) {
      text = substr(text, 0, text.size() - (a_rect.max.x - c_rect.max.x));
    }
    if (text.empty()) {
      return;
    }

    this->append_cmd_draw_text(c_rect.min, text, style);
  }

  /// Append command to draw text inside rect
  /// \warning \c text must exists until next im_renderer::start_new_frame(...) call
  void cmd_draw_text_in_rect(im_rect const& rect, std::span<std::uint32_t const> text, im_style const& style,
      im_halign halign = im_halign::left, im_valign valign = im_valign::top) {
    auto const a_rect = this->adjust(rect);
    auto const c_rect = clip_rect_.intersection(a_rect);
    if (!c_rect) {
      // nothing to draw
      return;
    }

    auto const text_length = static_cast<int>(text.size());
    auto const rect_width = static_cast<int>(a_rect.width());
    auto const rect_height = static_cast<int>(a_rect.height());

    int text_min_y;
    switch (valign) {
    case im_valign::top:
      text_min_y = a_rect.min.y;
      break;
    case im_valign::center:
      text_min_y = a_rect.min.y + (rect_height - 1) / 2;
      break;
    case im_valign::bottom:
      text_min_y = a_rect.min.y + (rect_height - 1);
      break;
    }
    auto text_max_y = text_min_y + 1 - 1;
    auto const invisible = (text_min_y < clip_rect_.min.y) || (text_max_y > clip_rect_.max.y);
    if (invisible) {
      // no more actions requires
      return;
    }

    int text_min_x;
    switch (halign) {
    case im_halign::left:
      text_min_x = a_rect.min.x;
      break;
    case im_halign::center:
      text_min_x = a_rect.min.x + (rect_width - text_length) / 2;
      break;
    case im_halign::right:
      text_min_x = a_rect.min.x + (rect_width - text_length);
      break;
    }
    auto text_max_x = text_min_x + text_length - 1;

    if (text_min_x < clip_rect_.min.x) {
      text = substr(text, clip_rect_.min.x - text_min_x);
      text_min_x = clip_rect_.min.x;
    }
    if (text_max_x > clip_rect_.max.x) {
      text = substr(text, 0, text.size() - (text_max_x - clip_rect_.max.x));
      text_max_x = clip_rect_.max.x;
    }

    if (text_min_x <= text_max_x) {
      this->append_cmd_draw_text(im_vec2(text_min_x, text_min_y), text, style);
    }
  }

  void cmd_draw_surface(im_vec2 const& pos, im_vec2 const& size, std::span<im_cell const> data) {
    auto const a_rect = this->adjust(im_rect(pos, pos + size - im_vec2(1, 1)));
    auto const c_rect = clip_rect_.intersection(a_rect);
    if (!c_rect) [[unlikely]] {
      return;
    }

    this->append_cmd_draw_surface(a_rect, c_rect, data);
  }

private:
  // adjust rectangle according to viewport offset
  [[nodiscard]] auto adjust(im_rect const& rect) noexcept -> im_rect {
    return im_rect(rect.min - viewport_offset_, rect.max - viewport_offset_);
  }

  void append_cmd_fill_rect(im_rect const& rect, std::uint32_t ch, im_style const& style) {
    auto& cmd = commands_.emplace_back();
    cmd.type = render_cmd_type::fill_rect;
    cmd.style = style;
    cmd.fill_rect_data = render_cmd_fill_rect_data{.rect = rect, .ch = ch};
  }

  void append_cmd_draw_rect(im_rect const& rect, im_style const& style) {
    auto& cmd = commands_.emplace_back();
    cmd.type = render_cmd_type::draw_rect;
    cmd.style = style;
    cmd.draw_rect_data = render_cmd_draw_rect_data{.rect = rect};
  }

  void append_cmd_draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) {
    auto& cmd = commands_.emplace_back();
    cmd.type = render_cmd_type::draw_text;
    cmd.style = style;
    cmd.draw_text_data = {};
    cmd.draw_text_data.pos = pos;
    cmd.draw_text_data.text = text;
  }

  void append_cmd_draw_surface(im_rect const& src_rect, im_rect const& rect, std::span<im_cell const> data) {
    auto& cmd = commands_.emplace_back();
    cmd.type = render_cmd_type::draw_surface;
    cmd.style = {};
    cmd.draw_surface_data = {.src_rect = src_rect, .rect = rect, .data = data};
  }

  static void do_fill_rect(render_cmd const& cmd);
  static void do_draw_rect(render_cmd const& cmd);
  static void do_draw_text(render_cmd const& cmd);
  static void do_draw_surface(render_cmd const& cmd);
};

} // namespace xxx
