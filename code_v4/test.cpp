// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <chrono>
#include <cmath>
#include <print>
#include <thread>

#include <xxx.h>

struct loop_rate_limiter {
  using clock = std::chrono::steady_clock;
  typename clock::duration max_loop_cycle_time;
  typename clock::time_point expected_stop_time_point;

  loop_rate_limiter(typename clock::duration max_cycle_time) noexcept : max_loop_cycle_time(max_cycle_time) {}

  void sleep() noexcept {
    auto now = clock::now();
    if (now < expected_stop_time_point) {
      std::this_thread::sleep_for(expected_stop_time_point - now);
      expected_stop_time_point += max_loop_cycle_time;
    } else {
      expected_stop_time_point = now + max_loop_cycle_time;
    }
  }

  void reset() noexcept {
    expected_stop_time_point = {};
  }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  using namespace xxx::literals;

  try {
    auto loop_limiter = loop_rate_limiter(std::chrono::milliseconds(1000) / 30);

    bool show_label_1 = false;
    bool show_label_2 = false;
    std::string string_value_1 = "str";
    std::string string_value_2 = "x1";

    xxx::init();
    xxx::set_default_color(xxx::im_color_id::text, {});
    xxx::set_default_color(xxx::im_color_id::background, {});

    while (true) {
      xxx::process_input_events();

      if (xxx::is_key_pressed(xxx::im_key_id::ctrl_c) || xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
        break;
      }

      xxx::new_frame();
      // xxx::debug();

      xxx::push_color(xxx::im_color_id::background, 0x4444ee_c);
      xxx::canvas_begin(32, 32);
      xxx::canvas_point(xxx::im_vec2(16, 16), 0x00ff00_c);
      xxx::canvas_point(xxx::im_vec2(16, 17), 0x00ff00_c);
      xxx::canvas_point(xxx::im_vec2(16, 18), 0x00ff00_c);
      xxx::canvas_point(xxx::im_vec2(1, 1), 0x00ff00_c);
      // for (float angle = 0.0; angle < 360.0; angle += 0.1) {
      //   auto const arg = angle * std::numbers::pi_v<float> / 180.0;
      //   int const x = 15 * std::cos(arg);
      //   int const y = 15 * std::sin(arg);
      //   xxx::canvas_point(xxx::im_vec2(16 + x, 16 + y), 0x0000ff_c);
      // }
      xxx::canvas_end();
      xxx::pop_color();

      xxx::view_begin("view1");
      xxx::label(string_value_1);
      xxx::label(string_value_2);
      xxx::layout_row_begin(3);
      xxx::layout_row_push(0.2);
      xxx::push_color(xxx::im_color_id::background, 0x111111_c);
      xxx::label("row 1 column 1");
      xxx::pop_color();
      xxx::layout_row_push(0.4);
      xxx::push_color(xxx::im_color_id::background, 0x222222_c);
      xxx::label("row 1 column 2");
      xxx::pop_color();
      xxx::layout_row_push(0.99);
      xxx::push_color(xxx::im_color_id::background, 0x333333_c);
      xxx::label("row 1 column 3 line 1");
      xxx::label("row 1 column 3 line 2");
      xxx::label("row 1 column 3 line 3");
      xxx::pop_color();
      xxx::layout_row_end();
      xxx::view_end();

      xxx::view_begin("view2", xxx::im_key_id::ctrl_f);
      xxx::layout_row_begin(3);
      xxx::layout_row_push(0.4);
      xxx::push_color(xxx::im_color_id::background, 0x444444_c);
      xxx::label("row 2 column 1 line 1");
      xxx::label("row 2 column 1 line 2");
      xxx::pop_color();
      xxx::layout_row_push(0.2);
      xxx::push_color(xxx::im_color_id::background, 0x555555_c);
      xxx::label("row 2 column 2");
      xxx::pop_color();
      xxx::layout_row_push(0.4);
      xxx::push_color(xxx::im_color_id::background, 0x666666_c);
      xxx::label("row 2 column 3 line 1");
      xxx::pop_color();
      xxx::layout_row_end();

      xxx::layout_row_begin(2);
      xxx::layout_row_push(0.5);
      if (xxx::button("show label##1")) {
        show_label_1 = !show_label_1;
      }
      xxx::layout_row_push(0.5);
      if (show_label_1) {
        xxx::label("this is first label");
      }
      xxx::layout_row_end();
      xxx::layout_row_begin(2);
      xxx::layout_row_push(0.5);
      if (xxx::button("show label##2")) {
        show_label_2 = !show_label_2;
      }
      xxx::layout_row_push(0.5);
      if (show_label_2) {
        xxx::label("this is second label");
      }
      xxx::layout_row_end();

      xxx::view_end();

      xxx::view_begin("view3", xxx::im_key_id::ctrl_g);
      xxx::layout_row_begin(2);
      xxx::layout_row_push(0.3);
      xxx::push_color(xxx::im_color_id::background, 0x777777_c);
      xxx::label("row 3 column 1 line 1");
      xxx::label("row 3 column 1 line 2");
      xxx::pop_color();
      xxx::layout_row_push(0.7);
      xxx::push_color(xxx::im_color_id::background, 0x888888_c);
      xxx::label("row 3 column 2 line 1");
      xxx::label("row 3 column 2 line 2");
      xxx::label("row 3 column 2 line 3");
      xxx::label("row 3 column 2 line 4");
      xxx::pop_color();
      xxx::layout_row_end();
      xxx::layout_row_begin(2);
      xxx::layout_row_push(0.4);
      if (xxx::text_input("enter value##1", string_value_1)) {
        string_value_1.clear();
      }
      xxx::layout_row_push(0.6);
      if (xxx::text_input("enter value##2", string_value_2)) {
        string_value_2.clear();
      }
      xxx::layout_row_end();
      xxx::view_end();

      xxx::push_color(xxx::im_color_id::background, 0xaa3333_c);
      xxx::label("end of layouts");
      xxx::pop_color();

      xxx::layout_row_begin(2);
      xxx::layout_row_push(0.4);
      xxx::view_begin("view4", xxx::im_view_flag_title);
      xxx::layout_row_begin(2);
      xxx::layout_row_push(0.7);
      xxx::panel_begin();
      xxx::label("abcd");
      xxx::label("123456");
      xxx::panel_end();
      xxx::layout_row_end();
      xxx::view_end();
      xxx::layout_row_push(0.6);
      xxx::push_color(xxx::im_color_id::background, 0x667755_c);
      xxx::view_begin("view5", xxx::im_view_flag_title);
      xxx::label("final layout");
      xxx::view_end();
      xxx::pop_color();
      xxx::layout_row_end();

      xxx::render();

      loop_limiter.sleep();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::print(stderr, "ERROR: {}\n", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
