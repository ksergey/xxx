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
    for (auto& layer : commands_) {
        layer.clear();
    }
    layer_ = 0;
}

void im_renderer::render(im_backend& backend) {
    backend.clear(clear_style_);

    for (auto const& cmd : commands_ | std::views::join) {
        switch (cmd.type) {
        case render_cmd_type::fill_rect: do_fill_rect(backend, cmd); break;
        case render_cmd_type::draw_rect: do_draw_rect(backend, cmd); break;
        case render_cmd_type::draw_text: do_draw_text(backend, cmd); break;
        case render_cmd_type::draw_surface: do_draw_surface(backend, cmd); break;
        default: break;
        }
    }

    backend.present();
}

void im_renderer::do_fill_rect(im_backend& backend, render_cmd const& cmd) {
    auto const& style = cmd.style;
    auto const& rect = cmd.fill_rect_data.rect;
    auto const& ch = cmd.fill_rect_data.ch;

    // plain loops: std::views::iota(begin, end) with begin > end doesn't stop (degenerate rects)
    for (int pos_y = rect.min.y; pos_y <= rect.max.y; ++pos_y) {
        for (int pos_x = rect.min.x; pos_x <= rect.max.x; ++pos_x) {
            backend.set_cell(pos_x, pos_y, ch, style);
        }
    }
}

void im_renderer::do_draw_rect(im_backend& backend, render_cmd const& cmd) {
    auto const& style = cmd.style;
    auto const& rect = cmd.draw_rect_data.rect;
    auto const& border_style = border_glyphs[static_cast<std::size_t>(cmd.draw_rect_data.border)];

    // plain loops: a rect one row / column high has no sides, and must not loop forever
    for (int pos_x = rect.min.x + 1; pos_x < rect.max.x; ++pos_x) {
        backend.set_cell(pos_x, rect.min.y, border_style[5], style);
        backend.set_cell(pos_x, rect.max.y, border_style[5], style);
    }
    for (int pos_y = rect.min.y + 1; pos_y < rect.max.y; ++pos_y) {
        backend.set_cell(rect.min.x, pos_y, border_style[4], style);
        backend.set_cell(rect.max.x, pos_y, border_style[4], style);
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
        // backends skip the cell covered by a wide char on present
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

    // plain indices: std::views::chunk (C++23) is missing in libc++ 18
    auto const src_width = src_rect.width();
    for (int y = 0; y < take_y; ++y) {
        auto const* const line = data.data() + std::size_t(drop_y + y) * std::size_t(src_width) + std::size_t(drop_x);
        for (int x = 0; x < take_x; ++x) {
            backend.set_cell(rect.min.x + x, rect.min.y + y, line[x].ch, line[x].style);
        }
    }
}

} // namespace xxx
