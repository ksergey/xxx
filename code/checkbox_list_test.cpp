// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <array>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "test_headless.h"

namespace xxx::testing {

using namespace std::string_view_literals;

TEST_SUITE("checkbox") {

    struct checkbox_app : headless_app {
        bool a = false, b = true;
        bool toggled_a = false, toggled_b = false;

        checkbox_app() : headless_app(im_vec2(20, 3)) {
            step();
        }
        void step() {
            frame([this] {
                view_begin("v", 0);
                toggled_a = checkbox("alpha", a);
                toggled_b = checkbox("beta", b);
                view_end();
            });
        }
    };

    TEST_CASE("rendering") {
        checkbox_app app;
        CHECK(app.screen() == "[ ] alpha\n[x] beta");
    }

    TEST_CASE("space / enter toggle focused checkbox") {
        checkbox_app app;
        app.backend().push_char(' ');
        app.step();
        CHECK(app.a);
        CHECK(app.toggled_a);
        CHECK_FALSE(app.toggled_b);
        CHECK(app.line(0) == "[x] alpha");

        app.step();
        CHECK_FALSE(app.toggled_a); // one frame

        app.backend().push_key(im_key_id::enter);
        app.step();
        CHECK_FALSE(app.a);
    }

    TEST_CASE("click toggles") {
        checkbox_app app;
        app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(6, 1));
        app.step();
        CHECK_FALSE(app.b);
        CHECK(app.toggled_b);
        CHECK(app.line(1) == "[ ] beta");
    }

    TEST_CASE("focus is shown by color") {
        checkbox_app app;
        CHECK(app.fg(4, 0) == test_active);
        CHECK(app.fg(4, 1) == test_inactive);
    }
}

TEST_SUITE("list") {

    constexpr auto fruits = std::to_array({"apple"sv, "banana"sv, "cherry"sv, "date"sv, "elder"sv, "fig"sv});

    struct list_app : headless_app {
        int selected = 0;
        int height = 3;
        bool activated = false;

        explicit list_app(int initial = 0) : headless_app(im_vec2(12, 6)), selected(initial) {
            step();
        }
        void step() {
            frame([this] {
                view_begin("v", 0);
                activated = list("fruits", fruits, selected, height);
                label("--");
                view_end();
            });
        }
        void key(im_key_id k, int times = 1) {
            for (int i = 0; i < times; ++i) {
                backend().push_key(k);
            }
            step();
        }
        // row of reversed (selected) line, -1 if none
        auto highlighted_row() const -> int {
            auto const cells = reversed_cells();
            return cells.empty() ? -1 : cells.front().y;
        }
    };

    TEST_CASE("rendering with height") {
        list_app app;
        CHECK(app.screen() == "apple\nbanana\ncherry\n--");
        CHECK(app.highlighted_row() == 0);
        CHECK(app.reversed_cells().size() == 12); // whole row
    }

    TEST_CASE("height 0 shows all items") {
        list_app app;
        app.height = 0;
        app.step();
        CHECK(app.line(5) == "fig");
    }

    TEST_CASE("arrows move selection and clamp") {
        list_app app;
        app.key(im_key_id::arrow_down);
        CHECK(app.selected == 1);
        app.key(im_key_id::arrow_up, 5);
        CHECK(app.selected == 0);
        app.key(im_key_id::arrow_down, 100);
        CHECK(app.selected == 5);
    }

    TEST_CASE("repeated arrows within one frame all count") {
        list_app app;
        app.key(im_key_id::arrow_down, 3);
        CHECK(app.selected == 3);
    }

    TEST_CASE("list scrolls to keep selection visible") {
        list_app app;
        app.key(im_key_id::arrow_down, 3);
        CHECK(app.screen() == "banana\ncherry\ndate\n--");
        CHECK(app.highlighted_row() == 2);

        app.key(im_key_id::end);
        CHECK(app.screen() == "date\nelder\nfig\n--");
        app.key(im_key_id::home);
        CHECK(app.screen() == "apple\nbanana\ncherry\n--");
    }

    TEST_CASE("moving inside visible rows does not scroll") {
        list_app app;
        app.key(im_key_id::arrow_down, 4);
        app.key(im_key_id::arrow_up);
        CHECK(app.line(0) == "cherry");
        CHECK(app.highlighted_row() == 1);
    }

    TEST_CASE("enter activates") {
        list_app app;
        app.key(im_key_id::arrow_down);
        CHECK_FALSE(app.activated);
        app.key(im_key_id::enter);
        CHECK(app.activated);
        CHECK(app.selected == 1);
    }

    TEST_CASE("click selects and activates item under mouse, respecting scroll") {
        list_app app;
        app.key(im_key_id::end); // shows date, elder, fig
        app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(2, 1));
        app.step();
        CHECK(app.selected == 4);
        CHECK(app.activated);
    }

    TEST_CASE("no selection") {
        list_app app(-1);
        CHECK(app.highlighted_row() == -1);
        app.key(im_key_id::enter);
        CHECK_FALSE(app.activated);
        app.key(im_key_id::arrow_down);
        CHECK(app.selected == 0);
    }

    TEST_CASE("out of range selection is clamped") {
        list_app app(42);
        CHECK(app.selected == 5);
    }

    TEST_CASE("empty list") {
        headless_app app(im_vec2(10, 3));
        int selected = 3;
        auto const ui = [&] {
            view_begin("v", 0);
            list("empty", std::span<std::string const>(), selected, 2);
            label("after");
            view_end();
        };
        app.frame(ui);
        app.backend().push_key(im_key_id::arrow_down);
        app.frame(ui);
        CHECK(selected == -1);
        CHECK(app.screen() == "\n\nafter");
    }

    TEST_CASE("std::string items and wide chars") {
        headless_app app(im_vec2(10, 3));
        auto const items = std::vector<std::string>{"日本", "x"};
        int selected = 0;
        app.frame([&] {
            view_begin("v", 0);
            list("l", items, selected);
            view_end();
        });
        CHECK(app.screen() == "日本\nx");
    }

    TEST_CASE("unfocused list shows selection in inactive color") {
        headless_app app(im_vec2(12, 4));
        int selected = 1;
        app.frame([&] {
            view_begin("v", 0);
            button("b");
            list("l", fruits, selected, 2);
            view_end();
        });
        // reverse means focus: the unfocused list underlines its selection instead
        CHECK_FALSE(app.reversed_in_row(2));
        REQUIRE(app.underlined_cells().size() == 12);
        CHECK(app.underlined_cells().front().y == 2);
        CHECK(app.fg(0, 2) == test_inactive);
    }
}

} // namespace xxx::testing
