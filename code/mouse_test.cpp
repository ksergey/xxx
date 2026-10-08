// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <format>
#include <string>

#include "test_headless.h"

namespace xxx::testing {

namespace {

constexpr auto left = im_mouse_button_id::left;

} // namespace

TEST_SUITE("mouse: buttons") {

    struct two_buttons : headless_app {
        bool a = false, b = false;

        two_buttons() : headless_app(im_vec2(20, 4)) {
            step();
        }
        void step() {
            frame([this] {
                view_begin("v", 0);
                a = button("a"); // row 0, cells 0..9
                b = button("b"); // row 1
                view_end();
            });
        }
        void click(im_vec2 pos, im_mouse_button_id button = left) {
            backend().push_mouse_button(button, pos);
            step();
        }
    };

    TEST_CASE("click presses button") {
        two_buttons app;
        app.click(im_vec2(5, 1));
        CHECK_FALSE(app.a);
        CHECK(app.b);
        app.step();
        CHECK_FALSE(app.b); // one frame
    }

    TEST_CASE("whole button area is clickable") {
        two_buttons app;
        app.click(im_vec2(0, 1));
        CHECK(app.b);
        app.click(im_vec2(9, 1));
        CHECK(app.b);
        app.click(im_vec2(10, 1)); // past the button
        CHECK_FALSE(app.b);
    }

    TEST_CASE("click moves keyboard focus") {
        two_buttons app;
        app.click(im_vec2(3, 1));
        app.backend().push_key(im_key_id::enter);
        app.step();
        CHECK(app.b);
        CHECK_FALSE(app.a);
        CHECK(app.fg(4, 1) == test_active);
        CHECK(app.fg(4, 0) == test_inactive);
    }

    TEST_CASE("right and middle clicks are ignored") {
        two_buttons app;
        app.click(im_vec2(3, 1), im_mouse_button_id::right);
        app.click(im_vec2(3, 1), im_mouse_button_id::middle);
        CHECK_FALSE(app.b);
        CHECK(app.fg(4, 0) == test_active); // focus unchanged
    }

    TEST_CASE("click before first frame hits nothing") {
        headless_app app(im_vec2(20, 2));
        bool pressed = false;
        app.backend().push_mouse_button(left, im_vec2(3, 0));
        app.frame([&] {
            view_begin("v", 0);
            pressed = button("a");
            view_end();
        });
        CHECK_FALSE(pressed);
    }

    TEST_CASE("buttons outside views are inert") {
        headless_app app(im_vec2(20, 2));
        bool pressed = false;
        auto const ui = [&] {
            pressed = button("a");
        };
        app.frame(ui);
        app.backend().push_mouse_button(left, im_vec2(3, 0));
        app.frame(ui);
        CHECK_FALSE(pressed);
    }
}

TEST_SUITE("mouse: views") {

    struct two_views : headless_app {
        bool a = false, b = false;

        two_views() : headless_app(im_vec2(40, 4)) {
            step();
        }
        void step() {
            frame([this] {
                layout_row_begin(2);
                layout_row_push(0.5);
                view_begin("left");
                a = button("a");
                view_end();
                layout_row_push(0.5);
                view_begin("right");
                label("text");
                b = button("b");
                view_end();
                layout_row_end();
            });
        }
        void click(im_vec2 pos) {
            backend().push_mouse_button(left, pos);
            step();
        }
    };

    TEST_CASE("click on inactive view activates it in the same frame") {
        two_views app;
        REQUIRE(app.fg(20, 0) == test_inactive);
        app.click(im_vec2(25, 1)); // on label
        CHECK(app.fg(20, 0) == test_active);
        CHECK(app.fg(0, 0) == test_inactive);
        CHECK_FALSE(app.b); // label is not a button
    }

    TEST_CASE("click on border activates view") {
        two_views app;
        app.click(im_vec2(39, 3));
        CHECK(app.fg(20, 0) == test_active);
    }

    TEST_CASE("click on button in inactive view activates view and presses button") {
        two_views app;
        app.click(im_vec2(25, 2));
        CHECK(app.b);
        CHECK(app.fg(20, 0) == test_active);
    }

    TEST_CASE("activating view by click resets focus to its first widget") {
        two_views app;
        app.click(im_vec2(25, 1));
        app.backend().push_key(im_key_id::enter);
        app.step();
        CHECK(app.b);
        CHECK_FALSE(app.a);
    }

    TEST_CASE("click outside views does nothing") {
        headless_app app(im_vec2(20, 6));
        auto const ui = [] {
            view_begin("a");
            view_end();
            view_begin("b");
            view_end();
        };
        app.frame(ui);
        app.backend().push_mouse_button(left, im_vec2(5, 5));
        app.frame(ui);
        CHECK(app.fg(0, 0) == test_active);
    }
}

TEST_SUITE("mouse: scrolled view") {

    TEST_CASE("only visible part of widget is clickable") {
        headless_app app(im_vec2(16, 6));
        int pressed = -1;
        auto const ui = [&] {
            pressed = -1;
            view_begin("v", im_view_flags_default, {}, 4); // 2 visible rows
            for (int i = 0; i < 6; ++i) {
                if (button(std::format("b{}", i))) {
                    pressed = i;
                }
            }
            view_end();
        };
        app.frame(ui);
        app.backend().push_key(im_key_id::page_down); // scroll by 1 row
        app.frame(ui);
        REQUIRE(app.line(1).find("[ b1 ]") != std::string::npos);

        app.backend().push_mouse_button(left, im_vec2(5, 1));
        app.frame(ui);
        CHECK(pressed == 1);

        // b0 is scrolled out: clicking on title border above must not press it
        app.backend().push_mouse_button(left, im_vec2(5, 0));
        app.frame(ui);
        CHECK(pressed == -1);
    }
}

TEST_SUITE("mouse: text_input") {

    struct input_app : headless_app {
        std::string text;

        explicit input_app(std::string initial) : headless_app(im_vec2(20, 3)), text(std::move(initial)) {
            step();
        }
        void step() {
            frame([this] {
                view_begin("v", 0);
                button("btn");          // row 0, focused first
                text_input("ph", text); // row 1, text starts at x = 2
                view_end();
            });
        }
        void click(int x) {
            backend().push_mouse_button(left, im_vec2(x, 1));
            step();
        }
        auto cursor() const -> int {
            auto const cells = reversed_cells();
            REQUIRE(cells.size() == 1);
            return cells[0].x - 2;
        }
    };

    TEST_CASE("click focuses input and places cursor") {
        input_app app("hello");
        REQUIRE_FALSE(app.reversed_in_row(1)); // the button has focus, the input shows no cursor
        app.click(2 + 2);                      // on first 'l'
        CHECK(app.cursor() == 2);
        app.backend().push_text("X");
        app.step();
        CHECK(app.text == "heXllo");
    }

    TEST_CASE("click on prompt puts cursor at start") {
        input_app app("hello");
        app.click(0);
        CHECK(app.cursor() == 0);
    }

    TEST_CASE("click past the end puts cursor at end") {
        input_app app("hi");
        app.click(12);
        CHECK(app.cursor() == 2);
    }

    TEST_CASE("click on already focused input moves cursor") {
        input_app app("hello");
        app.click(2);
        app.click(2 + 4);
        CHECK(app.cursor() == 4);
    }

    TEST_CASE("click on either half of wide char puts cursor before it") {
        input_app app("a日b");
        app.click(2 + 1); // left half of 日
        CHECK(app.cursor() == 1);
        app.click(2 + 2); // right half of 日
        CHECK(app.cursor() == 1);
        app.click(2 + 3); // 'b'
        CHECK(app.cursor() == 3);
    }
}

TEST_SUITE("im_rect contains") {
    TEST_CASE("point") {
        auto const r = im_rect(1, 1, 3, 2);
        CHECK(r.contains(im_vec2(1, 1)));
        CHECK(r.contains(im_vec2(3, 2)));
        CHECK_FALSE(r.contains(im_vec2(0, 1)));
        CHECK_FALSE(r.contains(im_vec2(4, 2)));
        CHECK_FALSE(r.contains(im_vec2(2, 3)));
        CHECK_FALSE(im_rect().contains(im_vec2(0, 0)));
    }
    TEST_CASE("rect") {
        auto const r = im_rect(0, 0, 9, 9);
        CHECK(r.contains(im_rect(2, 2, 3, 3)));
        CHECK(r.contains(r));
        CHECK_FALSE(r.contains(im_rect(5, 5, 10, 6)));
    }
}

} // namespace xxx::testing
