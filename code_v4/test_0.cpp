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
    auto loop_limiter = loop_rate_limiter(std::chrono::milliseconds(1000) / 60);

    std::string first_name;
    std::string second_name;
    std::string age;

    xxx::init();

    while (true) {
      xxx::process_input_events();

      if (xxx::is_key_pressed(xxx::im_key_id::ctrl_c) || xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
        break;
      }

      xxx::new_frame();

      xxx::layout_row_begin(2);
      xxx::layout_row_push(0.5);
      xxx::view_begin("demo");

      xxx::button("action 1##1");
      xxx::same_line();
      xxx::button("action 2##2");
      xxx::same_line();
      xxx::button("action 3##3");
      xxx::same_line();
      xxx::button("action 5##5");

      xxx::button("action 4##4");

      xxx::label("label 1");

      xxx::label("label 2");
      xxx::same_line();
      xxx::label("label 3");

      xxx::label("----");

      xxx::text_input("First name", first_name);
      xxx::same_line();
      xxx::text_input("Second name", second_name);
      xxx::text_input("Age", age);

      xxx::label("----");

      xxx::view_end();
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
