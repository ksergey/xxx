// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>

#include "test_headless.h"

namespace xxx::testing {

namespace {

constexpr auto left = im_mouse_button_id::left;

// main view with two buttons, "clear" opens confirmation popup
struct confirm_app : headless_app {
    int cleared = 0, other = 0, yes = 0, no = 0;

    confirm_app() : headless_app(im_vec2(30, 9)) {
        step();
    }
    void step() {
        frame([this] {
            view_begin("main", im_key_id::ctrl_a);
            if (button("clear")) {
                ++cleared;
                open_popup("confirm");
            }
            if (button("other")) {
                ++other;
            }
            label("background text here");
            view_end();

            if (popup_begin("confirm", "Clear?", 26)) {
                label("wipe the world?");
                if (button("yes")) {
                    ++yes;
                    close_popup();
                }
                same_line();
                if (button("no")) {
                    ++no;
                    close_popup();
                }
                popup_end();
            }
        });
    }
    void key(im_key_id k) {
        backend().push_key(k);
        step();
    }
    void click(im_vec2 pos) {
        backend().push_mouse_button(left, pos);
        step();
    }
    void open() {
        key(im_key_id::enter); // "clear" is focused
        REQUIRE(is_popup_open());
        step(); // first frame only measures the popup
    }
};

} // namespace

TEST_SUITE("popup") {

    TEST_CASE("closed popup builds nothing") {
        confirm_app app;
        CHECK_FALSE(is_popup_open());
        CHECK(app.line(5) == "");
    }

    TEST_CASE("first frame measures, then popup is centered on top") {
        confirm_app app;
        app.key(im_key_id::enter);
        CHECK(app.line(5) == ""); // measuring frame: not shown
        app.step();
        CHECK(app.screen() == dedent(R"(
      ╭──────── main <c-a> ────────╮
      │[ clear ]                   │
      │[╭──────── Clear? ────────╮ │
      │b│wipe the world?         │ │
      ╰─│ [ yes ]     [ no ]     │─╯
        ╰────────────────────────╯
    )"));
    }

    // regressions: both found while writing these tests
    TEST_CASE("key that opened popup does not press popup button") {
        confirm_app app;
        app.key(im_key_id::enter);
        CHECK(is_popup_open());
        CHECK(app.yes == 0);
    }

    TEST_CASE("widget after open_popup in the same frame does not take focus") {
        confirm_app app;
        app.open();
        app.key(im_key_id::enter); // goes to "yes", not to "other" in the main view
        CHECK(app.yes == 1);
        CHECK(app.other == 0);
    }

    TEST_CASE("views are inactive while popup is open") {
        confirm_app app;
        REQUIRE(app.fg(0, 0) == test_active);
        app.open();
        CHECK(app.fg(0, 0) == test_inactive);
        CHECK(app.fg(2, 2) == test_active); // popup frame
    }

    TEST_CASE("focus starts on first popup widget, tab cycles inside popup") {
        confirm_app app;
        app.open();
        CHECK(app.fg(6, 4) == test_active); // "yes"
        app.key(im_key_id::tab);
        CHECK(app.fg(18, 4) == test_active); // "no"
        app.key(im_key_id::tab);
        CHECK(app.fg(6, 4) == test_active); // wrapped, never left the popup
    }

    TEST_CASE("pressing button closes popup, focus returns") {
        confirm_app app;
        app.open();
        app.key(im_key_id::enter);
        CHECK(app.yes == 1);
        CHECK_FALSE(is_popup_open());
        app.step();
        CHECK(app.line(5) == "");
        CHECK(app.fg(0, 0) == test_active);

        // focus is back on "clear": enter opens popup again
        app.key(im_key_id::enter);
        CHECK(app.cleared == 2);
        CHECK(is_popup_open());
    }

    TEST_CASE("esc closes popup without pressing anything") {
        confirm_app app;
        app.open();
        app.key(im_key_id::esc);
        CHECK_FALSE(is_popup_open());
        CHECK(app.yes + app.no == 0);
    }

    TEST_CASE("clicks outside popup are ignored, inside work") {
        confirm_app app;
        app.open();
        app.click(im_vec2(1, 2)); // "other" button peeking out on the left
        CHECK(app.other == 0);
        CHECK(is_popup_open());
        app.click(im_vec2(20, 4)); // "no"
        CHECK(app.no == 1);
        CHECK_FALSE(is_popup_open());
    }

    TEST_CASE("keyboard does not reach views while popup is open") {
        confirm_app app;
        app.open();
        app.key(im_key_id::ctrl_a); // view shortcut
        app.key(im_key_id::tab);
        app.key(im_key_id::esc);
        app.step();
        CHECK(app.cleared == 1);
        CHECK(app.other == 0);
    }

    TEST_CASE("close_popup from outside") {
        confirm_app app;
        app.open();
        close_popup();
        app.step();
        CHECK(app.line(5) == "");
    }

    TEST_CASE("popup does not disturb surrounding layout") {
        headless_app app(im_vec2(20, 6));
        auto const ui = [] {
            label("one");
            if (popup_begin("p", "p", 10)) {
                label("x");
                label("y");
                popup_end();
            }
            label("two");
        };
        app.frame([&] {
            open_popup("p");
            ui();
        });
        app.frame(ui);
        CHECK(app.line(0) == "one");
        CHECK(app.line(1).starts_with("two ")); // popup is drawn over the rest of the line
    }

    TEST_CASE("height follows content") {
        headless_app app(im_vec2(20, 8));
        int lines = 1;
        auto const ui = [&] {
            if (popup_begin("p", "t", 10)) {
                for (int i = 0; i < lines; ++i) {
                    label("line");
                }
                popup_end();
            }
        };
        app.frame([&] {
            open_popup("p");
            ui();
        });
        app.frame(ui);
        // 3 rows high on 8 rows screen: top at row 2; 10 wide on 20: column 5
        CHECK(app.line(1) == "");
        CHECK(app.line(2) == "     ╭── t ───╮");
        CHECK(app.line(3) == "     │line    │");
        CHECK(app.line(4) == "     ╰────────╯");
        lines = 3;
        app.frame(ui); // drawn with new content right away, re-centered next frame
        app.frame(ui);
        CHECK(app.line(1) == "     ╭── t ───╮");
        CHECK(app.line(5) == "     ╰────────╯");
    }

    TEST_CASE("only one popup at a time") {
        headless_app app(im_vec2(20, 8));
        auto shown = std::string();
        auto const ui = [&] {
            shown.clear();
            for (auto const id : {"a", "b"}) {
                if (popup_begin(id, id, 10)) {
                    shown += id;
                    popup_end();
                }
            }
        };
        app.frame([&] {
            open_popup("a");
            ui();
        });
        CHECK(shown == "a");
        open_popup("b");
        app.frame(ui);
        CHECK(shown == "b");
    }

    TEST_CASE("wide chars in title") {
        headless_app app(im_vec2(20, 5));
        auto const ui = [] {
            if (popup_begin("p", "日本", 12)) {
                label("x");
                popup_end();
            }
        };
        app.frame([&] {
            open_popup("p");
            ui();
        });
        app.frame(ui);
        CHECK(app.line(1) == "    ╭── 日本 ──╮");
    }
}

} // namespace xxx::testing
