// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <format>
#include <string>

#include "test_headless.h"

namespace xxx::testing {

TEST_SUITE("id collisions") {

    TEST_CASE("unique ids") {
        headless_app app(im_vec2(20, 4));
        app.frame([] {
            view_begin("v", 0);
            button("ok");
            button("cancel");
            view_end();
        });
        CHECK(id_collision_count() == 0);
    }

    TEST_CASE("same label twice in one view") {
        headless_app app(im_vec2(20, 4));
        show_id_collisions(true);
        app.frame([] {
            view_begin("v", 0);
            button("ok");
            button("ok");
            view_end();
        });
        CHECK(id_collision_count() == 1);
        // both widgets are marked
        CHECK(app.line(0) == "! [ ok ]");
        CHECK(app.line(1) == "! [ ok ]");
        CHECK(app.backend().cell(0, 0).style.bg == 0xd70000u);
    }

    TEST_CASE("different widget types with the same label collide too") {
        headless_app app(im_vec2(20, 4));
        bool value = false;
        std::string text;
        app.frame([&] {
            view_begin("v", 0);
            checkbox("name", value);
            text_input("name", text);
            view_end();
        });
        CHECK(id_collision_count() == 1);
    }

    TEST_CASE("fixed with ## key or push_id") {
        headless_app app(im_vec2(20, 6));
        app.frame([] {
            view_begin("v", 0);
            button("ok##1");
            button("ok##2");
            for (int i = 0; i < 3; ++i) {
                push_id(i);
                button("delete");
                pop_id();
            }
            view_end();
        });
        CHECK(id_collision_count() == 0);
    }

    TEST_CASE("same label in different views is fine") {
        headless_app app(im_vec2(20, 8));
        app.frame([] {
            view_begin("a");
            button("ok");
            view_end();
            view_begin("b");
            button("ok");
            view_end();
        });
        CHECK(id_collision_count() == 0);
    }

    TEST_CASE("views with the same name collide") {
        headless_app app(im_vec2(20, 8));
        show_id_collisions(true);
        app.frame([] {
            view_begin("log");
            view_end();
            view_begin("log");
            view_end();
        });
        CHECK(id_collision_count() == 1);
        CHECK(app.line(0).starts_with("!"));
        CHECK(app.line(2).starts_with("!"));
    }

    TEST_CASE("several groups are counted separately") {
        headless_app app(im_vec2(20, 6));
        app.frame([] {
            view_begin("v", 0);
            button("a");
            button("a");
            button("a");
            button("b");
            button("b");
            view_end();
        });
        CHECK(id_collision_count() == 2);
    }

    TEST_CASE("count is per frame") {
        headless_app app(im_vec2(20, 4));
        bool twice = true;
        auto const ui = [&] {
            view_begin("v", 0);
            button("ok");
            if (twice) {
                button("ok");
            }
            view_end();
        };
        app.frame(ui);
        CHECK(id_collision_count() == 1);
        twice = false;
        app.frame(ui);
        CHECK(id_collision_count() == 0);
    }

    TEST_CASE("marker is hidden when disabled, count still works") {
        headless_app app(im_vec2(20, 4));
        show_id_collisions(false);
        app.frame([] {
            view_begin("v", 0);
            button("ok");
            button("ok");
            view_end();
        });
        CHECK(id_collision_count() == 1);
        CHECK(app.line(0) == "  [ ok ]");
    }

    TEST_CASE("marker respects clipping: scrolled out widget is not marked") {
        headless_app app(im_vec2(16, 6));
        show_id_collisions(true);
        app.frame([] {
            view_begin("v", im_view_flags_default, {}, 3); // one visible row
            button("x");
            button("y");
            button("x"); // below viewport
            view_end();
        });
        CHECK(id_collision_count() == 1);
        CHECK(app.line(1) == "│! [ x ]       ┃");
        CHECK(app.line(3) == ""); // nothing leaked below the view
    }

    TEST_CASE("marker is drawn above popup") {
        headless_app app(im_vec2(30, 8));
        show_id_collisions(true);
        auto const ui = [] {
            if (popup_begin("p", "p", 20)) {
                button("ok");
                button("ok");
                popup_end();
            }
        };
        app.frame([&] {
            open_popup("p");
            ui();
        });
        app.frame(ui);
        CHECK(id_collision_count() == 1);
        CHECK(app.line(3).find("│!") != std::string::npos);
    }
}

} // namespace xxx::testing
