// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <cstdio>
#include <exception>
#include <print>

#include <xxx.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    xxx::init(xxx::init_flags_mouse);

    bool show = true;
    bool done = false;

    while (!done) {
      xxx::update(std::chrono::milliseconds(1000) / 100);

      if (xxx::is_key_pressed(xxx::im_key_id::quit)) {
        break;
      }

      xxx::set_default_color(xxx::im_color_id::btn_fg, xxx::make_color(0.5, 0.5, 0.5));
      xxx::set_default_color(xxx::im_color_id::btn_bg, xxx::make_color(0.2, 0.2, 0.2));
      xxx::set_default_color(xxx::im_color_id::btn_fg_act, xxx::make_color(0.5, 1.0, 0.5));
      xxx::set_default_color(xxx::im_color_id::btn_bg_act, xxx::make_color(0.3, 0.3, 0.3));
      xxx::set_default_color(xxx::im_color_id::btn_fg_hov, xxx::make_color(1.0, 0.0, 0.9));
      xxx::set_default_color(xxx::im_color_id::btn_bg_hov, xxx::make_color(0.5, 0.5, 0.5));

      xxx::new_frame();
      // xxx::debug();

      xxx::push_color(xxx::im_color_id::fg, xxx::make_color(1.0f, 0.5f, 0.5f));
      xxx::label("stereo button");
      xxx::pop_color();

      if (xxx::button("toggle##1")) {
        show = !show;
      }

      if (show) {
        xxx::push_id("##show");
        xxx::push_color(xxx::im_color_id::fg, xxx::make_color(0.0f, 0.0f, 0.0f));
        xxx::push_color(xxx::im_color_id::bg, xxx::make_color(1.0f, 0.5f, 0.5f));
        xxx::label("stereo button");
        xxx::push_color(xxx::im_color_id::bg, xxx::make_color(0.3f, 0.6f, 0.9f));
        xxx::label("stereo button");
        xxx::pop_color(3);
        if (xxx::button("close##1")) {
          show = false;
        }
        xxx::pop_id();
      }
      xxx::label("stereo button");

      if (xxx::button("toggle##2")) {
        show = !show;
      }

      xxx::render();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::print(stderr, "ERROR: {}\n", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
