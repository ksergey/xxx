// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <chrono>
#include <thread>

#include <utility>

#include "test_headless.h"

namespace xxx::testing {

using namespace std::chrono_literals;

TEST_SUITE("event loop") {

    TEST_CASE("timeout without input: returns false, time passes") {
        headless_app app(im_vec2(10, 2));
        auto const t0 = app.backend().now();
        CHECK_FALSE(process_input_events(250ms));
        CHECK(app.backend().now() - t0 == 250ms);
    }

    TEST_CASE("input arrives: returns true, no waiting") {
        headless_app app(im_vec2(10, 2));
        auto const t0 = app.backend().now();
        app.backend().push_key(im_key_id::enter);
        CHECK(process_input_events(1000ms));
        CHECK(is_key_pressed(im_key_id::enter));
        CHECK(app.backend().now() == t0);
    }

    TEST_CASE("zero timeout does not wait") {
        headless_app app(im_vec2(10, 2));
        auto const t0 = app.backend().now();
        CHECK_FALSE(process_input_events(0ms));
        CHECK(app.backend().now() == t0);
    }

    TEST_CASE("wait_forever with input") {
        headless_app app(im_vec2(10, 2));
        app.backend().push_text("x");
        CHECK(process_input_events(wait_forever));
    }

    TEST_CASE("mouse counts as an event") {
        headless_app app(im_vec2(10, 2));
        app.backend().push_mouse_wheel(1, im_vec2(0, 0));
        CHECK(process_input_events(100ms));
    }

    TEST_CASE("processing works the same as non-blocking call") {
        headless_app app(im_vec2(20, 3));
        bool pressed = false;
        auto const ui = [&] {
            view_begin("v", 0);
            pressed = button("ok");
            view_end();
        };
        app.frame(ui);
        app.backend().push_key(im_key_id::enter);
        REQUIRE(process_input_events(500ms));
        new_frame();
        ui();
        render();
        CHECK(pressed);
    }

    TEST_CASE("wake_up from another thread ends the wait") {
        headless_app app(im_vec2(10, 2));
        auto const t0 = app.backend().now();
        auto worker = std::thread([] {
            wake_up();
        });
        worker.join();
        CHECK(process_input_events(wait_forever));
        CHECK(app.backend().now() == t0);       // didn't wait
        CHECK_FALSE(process_input_events(0ms)); // woken once
    }

    // anything deferred to the next frame must not wait for user input in an event driven loop
    TEST_CASE("deferred work requests the next frame") {
        headless_app app(im_vec2(30, 8));
        bool open = false;
        bool focus = false;
        auto const frame_after = [&](std::chrono::milliseconds timeout) {
            auto const t0 = app.backend().now();
            auto const woke = process_input_events(timeout);
            new_frame();
            view_begin("a", im_key_id::ctrl_a);
            view_end();
            view_begin("b", im_key_id::ctrl_b);
            if (std::exchange(focus, false)) {
                set_focus("x"); // id is resolved in the view scope
            }
            button("x");
            view_end();
            if (std::exchange(open, false)) {
                open_popup("p");
            }
            if (popup_begin("p", "p", 20)) {
                label("hello");
                popup_end();
            }
            render();
            return woke && app.backend().now() == t0; // returned at once, no waiting
        };
        frame_after(0ms);
        CHECK_FALSE(frame_after(1000ms)); // nothing deferred: waits for the timeout

        SUBCASE("popup: measuring frame is followed by a visible one") {
            open = true;
            frame_after(0ms);
            CHECK(app.screen().find("hello") == std::string::npos); // measured, hidden
            CHECK(frame_after(wait_forever));
            CHECK(app.screen().find("hello") != std::string::npos);
        }
        SUBCASE("view shortcut: activation is shown without more input") {
            app.backend().push_key(im_key_id::ctrl_b);
            frame_after(0ms);
            CHECK(frame_after(wait_forever));
            CHECK(app.fg(0, 2) == test_active);
        }
        SUBCASE("set_focus") {
            focus = true;
            frame_after(0ms); // matched while building: applied next frame
            CHECK(frame_after(wait_forever));
        }
    }

    TEST_CASE("animation is driven by waited time") {
        headless_app app(im_vec2(12, 1));
        float step = 0.0f;
        auto const frame_after = [&](std::chrono::milliseconds timeout) {
            process_input_events(timeout);
            new_frame();
            spinner("load", step);
            render();
        };
        frame_after(0ms);
        CHECK(app.line(0) == "⣽ load");
        frame_after(100ms); // spinner needs a tick every 100 ms, nothing else
        CHECK(app.line(0) == "⣻ load");
        frame_after(100ms);
        CHECK(app.line(0) == "⢿ load");
    }
}

} // namespace xxx::testing
