// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "im_renderer.h"

#include "im_backend.h"

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

void im_renderer::render(im_backend& backend) {
  backend.clear(clear_style_);

  for (auto const& cmd : commands_) {
    switch (cmd.type) {
    case render_cmd_type::fill_rect:
      do_fill_rect(backend, cmd);
      break;
    case render_cmd_type::draw_rect:
      do_draw_rect(backend, cmd);
      break;
    case render_cmd_type::draw_text:
      do_draw_text(backend, cmd);
      break;
    case render_cmd_type::draw_surface:
      do_draw_surface(backend, cmd);
      break;
    default:
      break;
    }
  }

  backend.present();
}

void im_renderer::do_fill_rect(im_backend& backend, render_cmd const& cmd) {
  auto const& style = cmd.style;
  auto const& rect = cmd.fill_rect_data.rect;
  auto const& ch = cmd.fill_rect_data.ch;

  for (int pos_x : std::views::iota(rect.min.x, rect.max.x + 1)) {
    for (int pos_y : std::views::iota(rect.min.y, rect.max.y + 1)) {
      backend.set_cell(pos_x, pos_y, ch, style);
    }
  }
}

void im_renderer::do_draw_rect(im_backend& backend, render_cmd const& cmd) {
  auto const& style = cmd.style;
  auto const& rect = cmd.draw_rect_data.rect;

  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::iota(rect.min.x + 1, rect.max.x),
           std::views::repeat(rect.min.y), std::views::repeat(border_style[5]))) {
    backend.set_cell(pos_x, pos_y, ch, style);
  }
  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::iota(rect.min.x + 1, rect.max.x),
           std::views::repeat(rect.max.y), std::views::repeat(border_style[5]))) {
    backend.set_cell(pos_x, pos_y, ch, style);
  }
  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::repeat(rect.min.x),
           std::views::iota(rect.min.y + 1, rect.max.y), std::views::repeat(border_style[4]))) {
    backend.set_cell(pos_x, pos_y, ch, style);
  }
  for (auto const& [pos_x, pos_y, ch] : std::views::zip(std::views::repeat(rect.max.x),
           std::views::iota(rect.min.y + 1, rect.max.y), std::views::repeat(border_style[4]))) {
    backend.set_cell(pos_x, pos_y, ch, style);
  }
  auto const& top_left = rect.top_left();
  backend.set_cell(top_left.x, top_left.y, border_style[0], style);
  auto const& top_right = rect.top_right();
  backend.set_cell(top_right.x, top_right.y, border_style[1], style);
  auto const& bottom_left = rect.bottom_left();
  backend.set_cell(bottom_left.x, bottom_left.y, border_style[2], style);
  auto const& bottom_right = rect.bottom_right();
  backend.set_cell(bottom_right.x, bottom_right.y, border_style[3], style);
}

void im_renderer::do_draw_text(im_backend& backend, render_cmd const& cmd) {
  auto const& style = cmd.style;
  auto const& pos = cmd.draw_text_data.pos;
  auto const& text = cmd.draw_text_data.text;

  auto pos_x = pos.x;
  if (cmd.draw_text_data.pad_left) {
    backend.set_cell(pos_x++, pos.y, ' ', style);
  }
  for (auto const ch : text) {
    backend.set_cell(pos_x, pos.y, ch, style);
    // termbox2 skips cells covered by wide char on present
    pos_x += char_width(ch);
  }
  if (cmd.draw_text_data.pad_right) {
    backend.set_cell(pos_x, pos.y, ' ', style);
  }
}

void im_renderer::do_draw_surface(im_backend& backend, render_cmd const& cmd) {
  auto const& src_rect = cmd.draw_surface_data.src_rect;
  auto const& rect = cmd.draw_surface_data.rect;
  auto const& data = cmd.draw_surface_data.data;

  // TODO: optimize?
  // if (src_rect == rect) {
  //   for (auto const& [pos_y, line] :
  //       std::views::zip(std::views::iota(src_rect.min.y), data | std::views::chunk(src_rect.width()))) {
  //     for (auto const& [pos_x, cell] : std::views::zip(std::views::iota(src_rect.min.x), line)) {
  //       backend.set_cell(pos_x, pos_y, cell.ch, cell.style);
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
      backend.set_cell(pos_x, pos_y, cell.ch, cell.style);
    }
  }
}

} // namespace xxx
