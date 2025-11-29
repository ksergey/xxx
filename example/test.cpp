// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <cstdio>
#include <exception>
#include <print>

#include <xxx-internal.h>
#include <xxx-layout.h>
#include <xxx-unicode.h>
#include <xxx.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    xxx::init(xxx::init_flags_mouse);

    bool done = false;

    while (!done) {
      xxx::update(std::chrono::milliseconds(25));

      if (xxx::is_key_pressed(xxx::im_key_id::quit)) {
        break;
      }

      xxx::new_frame();

      auto const ctx = xxx::get_context();
      auto& renderer = ctx->renderer;

      xxx::layout_row_begin(2);
      xxx::layout_row_push(0.4f);
      {
        xxx::layout_add_widget_item(5);
        auto& widget = ctx->widget;
        renderer.draw_rect(widget.bounds, xxx::make_color(1.0f, 1.0f, 0.0f));
      }
      {
        xxx::layout_add_widget_item(5);
        auto& widget = ctx->widget;
        renderer.draw_rect(widget.bounds, xxx::make_color(0.6f, 0.6f, 0.0f));
      }
      xxx::layout_row_push(0.6f);
      {
        xxx::layout_add_widget_item(3);
        auto& widget = ctx->widget;
        renderer.draw_rect(widget.bounds, xxx::make_color(0.0f, 1.0f, 1.0f));
      }
      {
        xxx::layout_add_widget_item(4);
        auto& widget = ctx->widget;
        renderer.draw_rect(widget.bounds, xxx::make_color(1.0f, 0.0f, 1.0f));
      }
      {
        xxx::layout_add_widget_item(2);
        auto& widget = ctx->widget;
        renderer.fill_rect(
            widget.bounds, ' ', xxx::im_style(xxx::make_color(0, 0, 0), xxx::make_color(0.2f, 0.2f, 0.2f)));
      }
      xxx::push_color(xxx::im_color_id::fg, xxx::make_color(1.0f, 0.0f, 0.0f));
      xxx::label("hello");
      xxx::pop_color();
      xxx::layout_row_end();
      {
        xxx::layout_add_widget_item(3);
        auto& widget = ctx->widget;
        renderer.draw_rect(widget.bounds, xxx::make_color(1.0f, 1.0f, 1.0f));
      }

      xxx::layout_container_begin(xxx::im_rect(20, 20, 25, 25));
      renderer.fill_rect(xxx::im_rect(0, 0, 100, 100), ' ',
          xxx::im_style(xxx::make_color(0, 0, 0), xxx::make_color(0.0f, 0.5f, 1.0f)));
      xxx::layout_container_end();

      xxx::render();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::print(stderr, "ERROR: {}\n", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
