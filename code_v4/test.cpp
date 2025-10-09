// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <chrono>
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

  auto view_label = [] {
    auto const name = xxx::view_get_name();
    auto const active = xxx::view_is_active();
    if (active) {
      xxx::push_color(xxx::im_color_id::text, 0x6666ff_c);
    }
    xxx::label(name);
    if (active) {
      xxx::pop_color();
    }
  };

  try {
    auto loop_limiter = loop_rate_limiter(std::chrono::milliseconds(1000) / 30);

    int border = 0;

    xxx::init();
    xxx::set_default_color(xxx::im_color_id::text, {});
    xxx::set_default_color(xxx::im_color_id::background, {});

    while (true) {
      xxx::process_input_events();

      if (xxx::is_key_pressed(xxx::im_key_id::quit)) {
        break;
      }
      if (xxx::is_key_pressed(xxx::im_key_id::enter)) {
        border = (border + 1) & 0x03;
      }

      xxx::new_frame();
      // xxx::debug();

      view_label();

      xxx::view_begin("view1");

      view_label();
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

      xxx::view_begin("view2");

      view_label();
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

      xxx::view_end();

      xxx::view_begin("view3");

      view_label();
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
      xxx::push_color(xxx::im_color_id::background, 0xaa3333_c);
      xxx::label("end of layouts");
      xxx::pop_color();

      xxx::view_end();

      xxx::view_begin("view4");
      view_label();
      xxx::view_end();

      view_label();

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
