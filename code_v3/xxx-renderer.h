// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

#include <termbox2.h>

#include "xxx-stack.h"
#include "xxx.h"

namespace xxx {

struct im_style {
  std::uint64_t fg = TB_DEFAULT;
  std::uint64_t bg = TB_DEFAULT;

  constexpr im_style() = default;
  constexpr im_style(im_color fg0, im_color bg0 = im_color(TB_DEFAULT)) noexcept
      : fg(static_cast<std::uint64_t>(fg0)), bg(static_cast<std::uint64_t>(bg0)) {}
};

enum class im_halign { left, center, right };
enum class im_valign { top, center, bottom };

struct im_renderer_none {};

struct im_renderer_rect_data {
  im_rect rect;
  std::uint32_t ch;
};

static constexpr auto im_renderer_text_max_size = std::size_t(32);

struct im_renderer_text_data {
  im_vec2 pos;
  std::array<std::uint32_t, im_renderer_text_max_size> text;
  std::size_t size;
};

enum class im_renderer_cmd_type { none, rect, text };

struct im_renderer_cmd {
  im_renderer_cmd_type type = im_renderer_cmd_type::none;
  im_style style;
  union {
    im_renderer_none none = {};
    im_renderer_rect_data rect;
    im_renderer_text_data text;
  };
};
static_assert(std::is_trivially_copyable_v<im_renderer_cmd>);

class im_renderer {
private:
  im_stack<im_rect> clip_rect_stack_;
  im_rect clip_rect_;
  std::vector<im_renderer_cmd> commands_;

public:
  im_renderer();
  ~im_renderer();

  [[nodiscard]] auto get_clip_rect() const noexcept -> im_rect const& {
    return clip_rect_;
  }

  void push_clip_rect(im_rect const& rect, bool clip_to_parent = true);

  void pop_clip_rect();

  void begin_frame();

  void end_frame();

  void present();

  void fill_rect(im_rect const& rect, std::uint32_t ch, im_style const& style);

  void draw_rect(im_rect const& rect, im_style const& style);

  void draw_text_at(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style);

  void draw_text_in_rect(im_rect const& rect, std::span<std::uint32_t const> text, im_style const& style,
      im_halign halign = im_halign::left, im_valign valign = im_valign::top);

private:
  void cmd_draw_rect(im_rect const& rect, std::uint32_t ch, im_style const& style);
  void cmd_draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style);

  static void do_draw_rect(im_renderer_cmd const& cmd);
  static void do_draw_text(im_renderer_cmd const& cmd);
};

} // namespace xxx
