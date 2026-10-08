// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <format>
#include <string>

#include "test_headless.h"

namespace xxx::testing {

namespace {

// two views side by side, three buttons each: a0 a1 a2 | b0 b1 b2
struct two_views : headless_app {
    std::string pressed;
    bool esc_seen = false;

    two_views() : headless_app(im_vec2(40, 8)) {
        step();
    }
    void step() {
        frame([this] {
            pressed.clear();
            esc_seen = is_key_pressed(im_key_id::esc);
            layout_row_begin(2);
            layout_row_push(ratio(0.5f));
            view_begin("a", im_key_id::ctrl_a);
            for (int i = 0; i < 3; ++i) {
                if (button(std::format("a{}", i))) {
                    pressed = std::format("a{}", i);
                }
            }
            view_end();
            layout_row_push(fill());
            view_begin("b", im_key_id::ctrl_b);
            for (int i = 0; i < 3; ++i) {
                if (button(std::format("b{}", i))) {
                    pressed = std::format("b{}", i);
                }
            }
            view_end();
            layout_row_end();
            key_hints();
        });
    }
    void key(im_key_id k) {
        backend().push_key(k);
        step();
        step(); // view switches by shortcut are applied on the next frame
    }
    auto focused() -> std::string {
        backend().push_key(im_key_id::enter);
        step();
        return pressed;
    }
};

} // namespace

TEST_SUITE("view focus memory") {

    TEST_CASE("returning to a view restores its focused widget") {
        two_views app;
        app.key(im_key_id::arrow_down);
        app.key(im_key_id::arrow_down);
        REQUIRE(app.focused() == "a2");
        app.key(im_key_id::ctrl_b);
        CHECK(app.focused() == "b0"); // never visited: first widget
        app.key(im_key_id::arrow_down);
        app.key(im_key_id::ctrl_a);
        CHECK(app.focused() == "a2"); // back where we were
        app.key(im_key_id::ctrl_b);
        CHECK(app.focused() == "b1");
    }

    TEST_CASE("shortcut of the active view goes to its top") {
        two_views app;
        app.key(im_key_id::arrow_down);
        app.key(im_key_id::arrow_down);
        app.key(im_key_id::ctrl_a);
        CHECK(app.focused() == "a0");
    }

    TEST_CASE("click on empty area of a view restores its widget") {
        two_views app;
        app.key(im_key_id::ctrl_b);
        app.key(im_key_id::arrow_down);
        app.key(im_key_id::ctrl_a);
        app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(35, 2)); // inside b, right of the buttons
        app.step();
        CHECK(app.focused() == "b1");
    }
}

TEST_SUITE("esc back") {

    TEST_CASE("Esc returns to the previous view and its widget") {
        two_views app;
        app.key(im_key_id::arrow_down); // a1
        app.key(im_key_id::ctrl_b);
        app.key(im_key_id::arrow_down); // b1
        app.key(im_key_id::esc);
        CHECK(app.focused() == "a1");
    }

    TEST_CASE("going back doesn't add history: Esc doesn't bounce") {
        two_views app;
        app.key(im_key_id::ctrl_b);
        app.key(im_key_id::esc); // back to a
        app.key(im_key_id::esc); // nothing left
        CHECK(app.focused() == "a0");
    }

    TEST_CASE("Esc with nothing to go back to reaches the application") {
        two_views app;
        app.backend().push_key(im_key_id::esc);
        app.step();
        CHECK(app.esc_seen);
    }

    TEST_CASE("Esc that went back is consumed") {
        two_views app;
        app.key(im_key_id::ctrl_b);
        app.backend().push_key(im_key_id::esc);
        app.step();
        CHECK_FALSE(app.esc_seen);
    }

    TEST_CASE("arrows between views are history too") {
        two_views app;
        app.key(im_key_id::arrow_right); // a0 -> b0
        app.key(im_key_id::esc);
        CHECK(app.focused() == "a0");
    }

    TEST_CASE("Esc in a popup only closes the popup") {
        two_views app;
        app.key(im_key_id::ctrl_b);
        app.frame([&] {
            open_popup("p");
            if (popup_begin("p", "p", 20)) {
                label("x");
                popup_end();
            }
        });
        REQUIRE(is_popup_open());
        app.backend().push_key(im_key_id::esc);
        app.frame([&] {
            if (popup_begin("p", "p", 20)) {
                label("x");
                popup_end();
            }
        });
        CHECK_FALSE(is_popup_open());
        app.step();
        CHECK(app.focused() == "b0"); // still in b
    }

    TEST_CASE("disabled") {
        two_views app;
        enable_esc_back(false);
        app.key(im_key_id::ctrl_b);
        app.backend().push_key(im_key_id::esc);
        app.step();
        CHECK(app.esc_seen);
        CHECK(app.focused() == "b0");
    }

    TEST_CASE("key hints mention Esc back only when it would work") {
        two_views app;
        CHECK(app.screen().find("Esc back") == std::string::npos);
        app.key(im_key_id::ctrl_b);
        CHECK(app.screen().find("Esc back") != std::string::npos);
    }
}

TEST_SUITE("shortcut twice on a log") {

    TEST_CASE("scrolls to the top") {
        headless_app app(im_vec2(20, 6));
        auto const ui = [] {
            view_begin("log", im_view_flags_default, im_key_id::ctrl_g, 4);
            for (int i = 0; i < 10; ++i) {
                label(std::format("line {}", i));
            }
            view_end();
        };
        app.frame(ui);
        for (int i = 0; i < 3; ++i) {
            app.backend().push_key(im_key_id::page_down);
            app.frame(ui);
        }
        REQUIRE(app.line(1).find("line 0") == std::string::npos);
        app.backend().push_key(im_key_id::ctrl_g);
        app.frame(ui);
        CHECK(app.line(1).find("line 0") != std::string::npos);
    }
}

} // namespace xxx::testing
