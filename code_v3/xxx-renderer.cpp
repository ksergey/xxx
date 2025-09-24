// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "xxx-renderer.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <ranges>

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

im_renderer::im_renderer() : clip_rect_stack_(32) {
  commands_.reserve(256);
}

im_renderer::~im_renderer() {}

void im_renderer::push_clip_rect(im_rect const& rect, bool clip_to_parent) {
  clip_rect_stack_.push_back(clip_rect_);
  clip_rect_ = clip_to_parent ? clip_rect_.intersection(rect) : rect;
}

void im_renderer::pop_clip_rect() {
  if (clip_rect_stack_.empty()) [[unlikely]] {
    return;
  }
  clip_rect_ = clip_rect_stack_.back();
  clip_rect_stack_.pop_back();
}

void im_renderer::begin_frame() {
  clip_rect_stack_.clear();
  clip_rect_ = im_rect(0, 0, ::tb_width() - 1, ::tb_height() - 1);
  commands_.clear();
}

void im_renderer::end_frame() {}

void im_renderer::present() {
  ::tb_clear();

  for (auto const& cmd : commands_) {
    switch (cmd.type) {
    case im_renderer_cmd_type::rect:
      do_draw_rect(cmd);
      break;
    case im_renderer_cmd_type::text:
      do_draw_text(cmd);
      break;
    default:
      break;
    }
  }

  ::tb_present();
}

void im_renderer::fill_rect(im_rect const& rect, std::uint32_t ch, im_style const& style) {
  auto const c_rect = rect.intersection(clip_rect_);
  if (!c_rect) [[unlikely]] {
    return;
  }

  this->cmd_draw_rect(c_rect, ch, style);
}

void im_renderer::draw_rect(im_rect const& rect, im_style const& style) {
  constexpr auto borders = std::to_array<std::uint32_t>({L'╭', L'╮', L'╰', L'╯', L'│', L'─'});

  if (!clip_rect_ || !rect) {
    return;
  }

  auto const top_left = rect.top_left();
  auto const top_right = rect.top_right();
  auto const bottom_left = rect.bottom_left();
  auto const bottom_right = rect.bottom_right();

  auto const hline_min_x = std::clamp<int>(top_left.x + 1, clip_rect_.min.x, clip_rect_.max.x);
  auto const hline_max_x = std::clamp<int>(top_right.x - 1, clip_rect_.min.x, clip_rect_.max.x);

  auto const vline_min_y = std::clamp<int>(top_left.y + 1, clip_rect_.min.y, clip_rect_.max.y);
  auto const vline_max_y = std::clamp<int>(bottom_left.y - 1, clip_rect_.min.y, clip_rect_.max.y);

  if (top_left.y >= clip_rect_.min.y) {
    if (top_left.x >= clip_rect_.min.x) {
      this->cmd_draw_rect(im_rect(top_left, top_left), borders[0], style);
    }
    if (top_right.x <= clip_rect_.max.x) {
      this->cmd_draw_rect(im_rect(top_right, top_right), borders[1], style);
    }
    if (hline_min_x < hline_max_x) {
      this->cmd_draw_rect(im_rect(hline_min_x, rect.min.y, hline_max_x, rect.min.y), borders[5], style);
    }
  }

  if (bottom_left.y <= clip_rect_.max.y) {
    if (bottom_left.x >= clip_rect_.min.x) {
      this->cmd_draw_rect(im_rect(bottom_left, bottom_left), borders[2], style);
    }
    if (bottom_right.x <= clip_rect_.max.x) {
      this->cmd_draw_rect(im_rect(bottom_right, bottom_right), borders[3], style);
    }
    if (hline_min_x < hline_max_x) {
      this->cmd_draw_rect(im_rect(hline_min_x, rect.max.y, hline_max_x, rect.max.y), borders[5], style);
    }
  }

  if (vline_min_y <= vline_max_y) {
    if (top_left.x >= clip_rect_.min.x) {
      this->cmd_draw_rect(im_rect(rect.min.x, vline_min_y, rect.min.x, vline_max_y), borders[4], style);
    }
    if (top_right.x <= clip_rect_.max.x) {
      this->cmd_draw_rect(im_rect(rect.max.x, vline_min_y, rect.max.x, vline_max_y), borders[4], style);
    }
  }
}

void im_renderer::draw_text_at(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) {
  auto const rect = im_rect(pos, pos + im_vec2(text.size() - 1, 0));
  auto const c_rect = rect.intersection(clip_rect_);
  if (!c_rect) {
    return;
  }

  if (c_rect.min.x > rect.min.x) {
    text = text.subspan(c_rect.min.x - rect.min.x);
  }
  if (c_rect.max.x < rect.max.x) {
    text = text.subspan(0, text.size() - (rect.max.x - c_rect.max.x));
  }
  if (text.empty()) {
    return;
  }

  this->cmd_draw_text(c_rect.min, text, style);
}

void im_renderer::draw_text_in_rect(im_rect const& rect, std::span<std::uint32_t const> text, im_style const& style,
    im_halign halign, im_valign valign) {
  auto const c_rect = rect.intersection(clip_rect_);
  if (!c_rect) {
    // nothing to draw
    return;
  }

  auto const text_length = static_cast<int>(text.size());
  auto const rect_width = static_cast<int>(rect.width());
  auto const rect_height = static_cast<int>(rect.height());

  int text_min_y;
  switch (valign) {
  case im_valign::top:
    text_min_y = rect.min.y;
    break;
  case im_valign::center:
    text_min_y = rect.min.y + (rect_height - 1) / 2;
    break;
  case im_valign::bottom:
    text_min_y = rect.min.y + (rect_height - 1);
    break;
  }
  auto text_max_y = text_min_y + 1 - 1;
  auto const invisible = (text_min_y < clip_rect_.min.y) || (text_max_y > clip_rect_.max.y);
  if (invisible) {
    // no more actions requires
    return this->cmd_draw_rect(c_rect, ' ', style);
  }

  int text_min_x;
  switch (halign) {
  case im_halign::left:
    text_min_x = rect.min.x;
    break;
  case im_halign::center:
    text_min_x = rect.min.x + (rect_width - text_length) / 2;
    break;
  case im_halign::right:
    text_min_x = rect.min.x + (rect_width - text_length);
    break;
  }
  auto text_max_x = text_min_x + text_length - 1;

  if (text_min_x < clip_rect_.min.x) {
    text = text.subspan(clip_rect_.min.x - text_min_x);
    text_min_x = clip_rect_.min.x;
  }
  if (text_max_x > clip_rect_.max.x) {
    text = text.subspan(0, text.size() - (text_max_x - clip_rect_.max.x));
    text_max_x = clip_rect_.max.x;
  }

  if (c_rect.min.y < text_min_y) {
    // top box
    this->cmd_draw_rect(im_rect(c_rect.min.x, c_rect.min.y, c_rect.max.x, text_min_y - 1), ' ', style);
  }

  // text box
  if (c_rect.min.x < text_min_x - 1) {
    this->cmd_draw_rect(im_rect(c_rect.min.x, text_min_y, text_min_x - 1, text_max_y), ' ', style);
  }
  if (text_min_x < text_max_x) {
    this->cmd_draw_text(im_vec2(text_min_x, text_min_y), text, style);
  }
  if (text_max_x + 1 < c_rect.max.x) {
    this->cmd_draw_rect(im_rect(text_max_x + 1, text_min_y, c_rect.max.x, text_max_y), ' ', style);
  }

  // bottom box
  if (text_max_y + 1 < c_rect.max.y) {
    this->cmd_draw_rect(im_rect(c_rect.min.x, text_max_y + 1, c_rect.max.x, c_rect.max.y), ' ', style);
  }
}

void im_renderer::cmd_draw_rect(im_rect const& rect, std::uint32_t ch, im_style const& style) {
  auto& cmd = commands_.emplace_back();
  cmd.type = im_renderer_cmd_type::rect;
  cmd.style = style;
  cmd.rect = im_renderer_rect_data{.rect = rect, .ch = ch};
}

void im_renderer::cmd_draw_text(im_vec2 const& pos, std::span<std::uint32_t const> text, im_style const& style) {
  im_vec2 text_pos = pos;
  for (std::span<std::uint32_t const> sub : text | std::views::chunk(im_renderer_text_max_size)) {
    auto& cmd = commands_.emplace_back();
    cmd.type = im_renderer_cmd_type::text;
    cmd.style = style;
    cmd.text = {};
    cmd.text.pos = text_pos;
    std::copy_n(sub.data(), sub.size(), cmd.text.text.data());
    cmd.text.size = sub.size();

    text_pos.x += sub.size();
  }
}

void im_renderer::do_draw_rect(im_renderer_cmd const& cmd) {
  auto const& style = cmd.style;
  auto const& rect = cmd.rect.rect;
  auto const& ch = cmd.rect.ch;

  for (int pos_x : std::views::iota(rect.min.x, rect.max.x + 1)) {
    for (int pos_y : std::views::iota(rect.min.y, rect.max.y + 1)) {
      ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
    }
  }
}

void im_renderer::do_draw_text(im_renderer_cmd const& cmd) {
  auto const& style = cmd.style;
  auto const& pos = cmd.text.pos;
  auto text = std::span<std::uint32_t const>(cmd.text.text.data(), cmd.text.size);

  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::iota(pos.x), std::views::repeat(pos.y), text)) {
    ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
  }
}

} // namespace xxx
