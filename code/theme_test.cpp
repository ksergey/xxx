// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "test_headless.h"

namespace xxx::testing {

TEST_SUITE("theme API") {

    TEST_CASE("label with a role and with a custom style") {
        headless_app app(im_vec2(20, 3));
        app.frame([] {
            label("lost", im_role::error);
            label("12 ms", {.fg = ansi::green, .bg = {}, .attrs = im_attr_bold});
            label("plain");
        });
        CHECK(app.backend().cell(0, 0).style.fg == ansi::red.value); // default theme: error is red
        CHECK(app.backend().cell(0, 1).style.fg == ansi::green.value);
        CHECK(app.backend().cell(0, 1).style.attrs == im_attr_bold);
        CHECK(im_color(app.backend().cell(0, 2).style.fg).is_default());
    }

    TEST_CASE("scoped_style restores at the end of the scope") {
        headless_app app(im_vec2(20, 3));
        app.frame([] {
            {
                auto const s = scoped_style(im_role::text, {.fg = ansi::magenta, .bg = {}, .attrs = 0});
                label("inside");
            }
            label("outside");
        });
        CHECK(app.backend().cell(0, 0).style.fg == ansi::magenta.value);
        CHECK(im_color(app.backend().cell(0, 1).style.fg).is_default());
    }

    TEST_CASE("theme background applies to every widget") {
        headless_app app(im_vec2(20, 3));
        set_style(im_role::text, {.fg = im_color(), .bg = im_color(0x102030u), .attrs = 0});
        app.frame([] {
            view_begin("v", 0);
            button("ok");
            label("x", im_role::error);
            view_end();
        });
        CHECK(app.backend().cell(2, 0).style.bg == 0x102030u); // button bracket
        CHECK(app.backend().cell(0, 1).style.bg == 0x102030u);
        CHECK(app.backend().cell(15, 2).style.bg == 0x102030u); // empty screen too
    }

    TEST_CASE("push_style leftovers don't leak into the next frame") {
        headless_app app(im_vec2(20, 2));
        app.frame([] {
            push_style(im_role::text, {.fg = ansi::red, .bg = {}, .attrs = 0}); // never popped
            label("a");
        });
        app.frame([] {
            label("b");
        });
        CHECK(im_color(app.backend().cell(0, 0).style.fg).is_default());
    }

    TEST_CASE("get_style resolves unset colors") {
        headless_app app(im_vec2(10, 1));
        set_style(im_role::text, {.fg = im_color(0x111111u), .bg = im_color(0x222222u), .attrs = 0});
        set_style(im_role::warning, {.fg = ansi::yellow, .bg = {}, .attrs = 0});
        auto const s = get_style(im_role::warning);
        CHECK(*s.fg == ansi::yellow);
        CHECK(*s.bg == im_color(0x222222u));
    }

    TEST_CASE("get_theme_style keeps unset colors unset") {
        headless_app app(im_vec2(10, 1));
        set_style(im_role::warning, {.fg = ansi::yellow, .bg = {}, .attrs = 0});
        auto const s = get_theme_style(im_role::warning);
        CHECK(*s.fg == ansi::yellow);
        CHECK_FALSE(s.bg.has_value()); // still follows text
        set_style(im_role::text, {.fg = im_color(), .bg = ansi::blue, .attrs = 0});
        CHECK(*get_style(im_role::warning).bg == ansi::blue);
    }

    TEST_CASE("use_theme") {
        headless_app app(im_vec2(10, 1));
        use_theme(im_theme_preset::classic);
        CHECK(*get_style(im_role::accent).fg == im_color(0xdcf763u));
        use_theme(im_theme_preset::terminal);
        CHECK(*get_style(im_role::accent).fg == ansi::cyan);
    }
}

} // namespace xxx::testing
