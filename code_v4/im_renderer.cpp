// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "im_renderer.h"

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

void im_renderer::start_new_frame(im_rect const& clip_rect) {
  clip_rect_stack_.clear();
  clip_rect_ = clip_rect;
  viewport_offset_ = im_vec2(0, 0);
  commands_.clear();
}

void im_renderer::render() {
  ::tb_clear();

  for (auto const& cmd : commands_) {
    switch (cmd.type) {
    case render_cmd_type::fill_rect:
      this->do_fill_rect(cmd);
      break;
    case render_cmd_type::draw_rect:
      this->do_draw_rect(cmd);
      break;
    case render_cmd_type::draw_text:
      this->do_draw_text(cmd);
      break;
    case render_cmd_type::draw_surface:
      this->do_draw_surface(cmd);
      break;
    default:
      break;
    }
  }

  ::tb_present();
}

void im_renderer::do_fill_rect(render_cmd const& cmd) {
  auto const& style = cmd.style;
  auto const& rect = cmd.fill_rect_data.rect;
  auto const& ch = cmd.fill_rect_data.ch;

  for (int pos_x : std::views::iota(rect.min.x, rect.max.x + 1)) {
    for (int pos_y : std::views::iota(rect.min.y, rect.max.y + 1)) {
      ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
    }
  }
}

void im_renderer::do_draw_rect(render_cmd const& cmd) {
  auto const& style = cmd.style;
  auto const& rect = cmd.draw_rect_data.rect;

  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::iota(rect.min.x + 1, rect.max.x),
           std::views::repeat(rect.min.y), std::views::repeat(border_style[5]))) {
    ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
  }
  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::iota(rect.min.x + 1, rect.max.x),
           std::views::repeat(rect.max.y), std::views::repeat(border_style[5]))) {
    ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
  }
  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::repeat(rect.min.x),
           std::views::iota(rect.min.y + 1, rect.max.y), std::views::repeat(border_style[4]))) {
    ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
  }
  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::repeat(rect.max.x),
           std::views::iota(rect.min.y + 1, rect.max.y), std::views::repeat(border_style[4]))) {
    ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
  }
  auto const& top_left = rect.top_left();
  ::tb_set_cell(top_left.x, top_left.y, border_style[0], style.fg, style.bg);
  auto const& top_right = rect.top_right();
  ::tb_set_cell(top_right.x, top_right.y, border_style[1], style.fg, style.bg);
  auto const& bottom_left = rect.bottom_left();
  ::tb_set_cell(bottom_left.x, bottom_left.y, border_style[2], style.fg, style.bg);
  auto const& bottom_right = rect.bottom_right();
  ::tb_set_cell(bottom_right.x, bottom_right.y, border_style[3], style.fg, style.bg);
}

void im_renderer::do_draw_text(render_cmd const& cmd) {
  auto const& style = cmd.style;
  auto const& pos = cmd.draw_text_data.pos;
  auto const& text = cmd.draw_text_data.text;

  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::iota(pos.x), std::views::repeat(pos.y), text)) {
    ::tb_set_cell(pos_x, pos_y, ch, style.fg, style.bg);
  }
}

void im_renderer::do_draw_surface(render_cmd const& cmd) {
  auto const& src_rect = cmd.draw_surface_data.src_rect;
  auto const& rect = cmd.draw_surface_data.rect;
  auto const& data = cmd.draw_surface_data.data;

  // TODO: optimize?
  // if (src_rect == rect) {
  //   for (auto const& [pos_y, line] :
  //       std::views::zip(std::views::iota(src_rect.min.y), data | std::views::chunk(src_rect.width()))) {
  //     for (auto const& [pos_x, cell] : std::views::zip(std::views::iota(src_rect.min.x), line)) {
  //       ::tb_set_cell(pos_x, pos_y, cell.ch, cell.style.fg, cell.style.bg);
  //     }
  //   }
  //   return;
  // }

  auto const drop_x = rect.min.x - src_rect.min.x;
  auto const take_x = rect.width();
  auto const drop_y = rect.min.y - src_rect.min.y;
  auto const take_y = rect.height();

  for (auto const& [pos_y, line] :
      std::views::zip(std::views::iota(rect.min.y), data | std::views::chunk(src_rect.width())) |
          std::views::drop(drop_y) | std::views::take(take_y)) {
    for (auto const& [pos_x, cell] :
        std::views::zip(std::views::iota(rect.min.x), line | std::views::drop(drop_x) | std::views::take(take_x))) {
      ::tb_set_cell(pos_x, pos_y, cell.ch, cell.style.fg, cell.style.bg);
    }
  }
}

} // namespace xxx
