// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>

#include "test_headless.h"

namespace xxx::testing {

namespace {

struct help_app : headless_app {
    std::string text = "abc";
    bool use_input = false;
    int pressed = 0;

    help_app() : headless_app(im_vec2(90, 40)) {
        step();
    }
    void step() {
        frame([this] {
            key_hint("c-q", "quit");
            view_begin("main", im_key_id::ctrl_o);
            if (use_input) {
                text_input("t", text);
            }
            if (button("ok")) {
                ++pressed;
            }
            view_end();
            view_begin("log", im_key_id::ctrl_e);
            view_end();
            key_hints("c-q quit");
        });
    }
    void key(im_key_id k) {
        backend().push_key(k);
        step();
    }
    void type(std::string_view s) {
        backend().push_text(s);
        step();
    }
    void focus_input() {
        use_input = true;
        step();
        key(im_key_id::arrow_up); // focus stays on "ok" when the input appears: move it there
    }
    auto hints() const -> std::string {
        for (int y = 0; y < 40; ++y) {
            if (auto const l = line(y);
                l.find("arrows move") != std::string::npos || l.find("Esc close") != std::string::npos) {
                return l;
            }
        }
        return {};
    }
    auto shows(std::string_view needle) const -> bool {
        return screen().find(needle) != std::string::npos;
    }
};

} // namespace

TEST_SUITE("help") {

    TEST_CASE("F1 opens help with app keys, view shortcuts and navigation") {
        help_app app;
        app.key(im_key_id::f1);
        app.step(); // measuring frame of the popup
        CHECK(app.shows(" keys "));
        CHECK(app.shows("quit"));       // key_hint
        CHECK(app.shows("go to main")); // view shortcuts, collected automatically
        CHECK(app.shows("go to log"));
        CHECK(app.shows("c-o"));
        CHECK(app.shows("move focus")); // navigation keys
        CHECK(is_popup_open());
    }

    TEST_CASE("F1 again and Esc close it") {
        help_app app;
        app.key(im_key_id::f1);
        app.key(im_key_id::f1);
        CHECK_FALSE(is_popup_open());
        app.key(im_key_id::f1);
        app.key(im_key_id::esc);
        CHECK_FALSE(is_popup_open());
    }

    TEST_CASE("? opens help when no text input has focus") {
        help_app app;
        app.type("?");
        CHECK(is_popup_open());
        app.type("?");
        CHECK_FALSE(is_popup_open());
    }

    TEST_CASE("? is just a character in a text input") {
        help_app app;
        app.focus_input();
        REQUIRE(wants_text_input());
        app.type("?");
        CHECK_FALSE(is_popup_open());
        CHECK(app.text == "abc?");
        app.key(im_key_id::f1); // F1 still works
        CHECK(is_popup_open());
    }

    TEST_CASE("key that opened help doesn't reach widgets") {
        help_app app;
        app.type("?");
        app.key(im_key_id::esc);
        CHECK(app.pressed == 0);
    }

    TEST_CASE("disabled") {
        help_app app;
        enable_help(false);
        app.key(im_key_id::f1);
        app.type("?");
        CHECK_FALSE(is_popup_open());
    }

    TEST_CASE("does not replace an open application popup") {
        help_app app;
        auto const ui = [&] {
            if (popup_begin("mine", "mine", 20)) {
                label("x");
                popup_end();
            }
        };
        app.frame([&] {
            open_popup("mine");
            ui();
        });
        app.backend().push_key(im_key_id::f1);
        app.frame(ui);
        app.frame(ui);
        CHECK(app.screen().find(" mine ") != std::string::npos);
        CHECK(app.screen().find(" keys ") == std::string::npos);
    }
}

TEST_SUITE("key_hints") {

    TEST_CASE("depend on the focused widget") {
        help_app app;
        CHECK(app.hints() == "Enter press · arrows move · F1 ? help · c-q quit");
        app.focus_input();
        CHECK(app.hints() == "^U ^K ^W cut · ^Y paste · ^C copy · arrows move · F1 help · c-q quit");
    }

    TEST_CASE("popup: Esc close") {
        help_app app;
        app.key(im_key_id::f1);
        app.step();
        CHECK(app.hints().find("Esc close") != std::string::npos);
    }

    TEST_CASE("wants_text_input") {
        help_app app;
        CHECK_FALSE(wants_text_input());
        app.focus_input();
        CHECK(wants_text_input());
    }
}

} // namespace xxx::testing
