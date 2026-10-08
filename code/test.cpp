// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <chrono>
#include <cmath>
#include <numbers>
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

        bool show_label_1 = false;
        bool show_label_2 = false;
        std::string string_value_1 = "str";
        std::string string_value_2 = "x1";
        std::string string_value_3 = "";

        xxx::init();
        xxx::set_style(xxx::im_role::text, {.fg = xxx::im_color(), .bg = xxx::im_color(), .attrs = 0});

        while (true) {
            xxx::process_input_events();

            if (xxx::is_key_pressed(xxx::im_key_id::ctrl_c) || xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
                break;
            }

            xxx::new_frame();
            // xxx::debug();

            xxx::spinner("first spinner");

            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x4444ee_c, .attrs = 0});
            if (xxx::canvas_begin(xxx::im_vec2{32, 32})) {
                for (float angle = 0.0; angle < 360.0; angle += 0.1) {
                    auto const arg = angle * std::numbers::pi_v<float> / 180.0;
                    int const x = 15 * std::cos(arg);
                    int const y = 15 * std::sin(arg);
                    xxx::canvas_point(xxx::im_vec2(15 + x, 15 + y), 0x33ff99_c);
                }
                xxx::canvas_end();
            }
            xxx::pop_style();

            xxx::view_begin("view1");
            xxx::label(string_value_1);
            xxx::label(string_value_2);
            xxx::layout_row_begin(3);
            xxx::layout_row_push(0.2);
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x111111_c, .attrs = 0});
            xxx::label("row 1 column 1");
            xxx::pop_style();
            xxx::layout_row_push(0.4);
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x222222_c, .attrs = 0});
            xxx::label("row 1 column 2");
            xxx::pop_style();
            xxx::layout_row_push(0.99);
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x333333_c, .attrs = 0});
            xxx::label("row 1 column 3 line 1");
            xxx::label("row 1 column 3 line 2");
            xxx::label("row 1 column 3 line 3");
            xxx::pop_style();
            xxx::layout_row_end();
            xxx::view_end();

            xxx::view_begin("view2", xxx::im_key_id::ctrl_f);
            xxx::layout_row_begin(3);
            xxx::layout_row_push(0.4);
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x444444_c, .attrs = 0});
            xxx::label("row 2 column 1 line 1");
            xxx::label("row 2 column 1 line 2");
            xxx::pop_style();
            xxx::layout_row_push(0.2);
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x555555_c, .attrs = 0});
            xxx::label("row 2 column 2");
            xxx::pop_style();
            xxx::layout_row_push(0.4);
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x666666_c, .attrs = 0});
            xxx::label("row 2 column 3 line 1");
            xxx::pop_style();
            xxx::layout_row_end();

            xxx::layout_row_begin(2);
            xxx::layout_row_push(0.5);
            if (xxx::button("show label##1")) {
                show_label_1 = !show_label_1;
            }
            xxx::layout_row_push(0.5);
            if (show_label_1) {
                xxx::label("this is first label");
                xxx::spinner<struct L1>("label1 spinner");
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
                xxx::spinner<struct L2>("label2 spinner");
            }
            xxx::layout_row_end();
            xxx::view_end();

            xxx::view_begin("view4", xxx::im_key_id::ctrl_t);
            if (xxx::button("Button 1##1")) {}
            if (xxx::button("Button 2##2")) {}
            if (xxx::button("Button 3##3")) {}
            xxx::view_end();

            xxx::view_begin("view3", xxx::im_key_id::ctrl_g);
            xxx::layout_row_begin(2);
            xxx::layout_row_push(0.3);
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x777777_c, .attrs = 0});
            xxx::label("row 3 column 1 line 1");
            xxx::label("row 3 column 1 line 2");
            xxx::pop_style();
            xxx::layout_row_push(0.7);
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x888888_c, .attrs = 0});
            xxx::label("row 3 column 2 line 1");
            xxx::label("row 3 column 2 line 2");
            xxx::label("row 3 column 2 line 3");
            xxx::label("row 3 column 2 line 4");
            xxx::pop_style();
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
            if (xxx::text_input("##4", string_value_3)) {}
            xxx::layout_row_end();
            xxx::view_end();

            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0xaa3333_c, .attrs = 0});
            xxx::label("end of layouts");
            xxx::pop_style();

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
            xxx::push_style(xxx::im_role::text, {.fg = {}, .bg = 0x667755_c, .attrs = 0});
            xxx::view_begin("view5", xxx::im_view_flag_title);
            xxx::label("final layout");
            xxx::view_end();
            xxx::pop_style();
            xxx::layout_row_end();

            xxx::layout_row_begin(2);
            xxx::layout_row_push(20);
            xxx::progress(5.0);
            xxx::progress(20.0);
            xxx::progress(40.0);
            xxx::progress(60.0);
            xxx::progress(80.0);
            xxx::progress(100.0);
            xxx::progress(110.0);
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
