// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <array>
#include <string>
#include <string_view>

#include "test_headless.h"

namespace xxx::testing {

using namespace std::string_view_literals;

namespace {

constexpr auto panel_bg = im_color(0x141414u);
constexpr auto screen_bg = im_color(0x0a0a0au);

[[nodiscard]] constexpr auto panel() -> im_view_look {
    return {.border = im_border::none, .bg = panel_bg, .pad_x = 1, .pad_y = 1, .bar_left = true};
}

} // namespace

TEST_SUITE("panel look") {

    TEST_CASE("frameless panel: bar, padding, title inside, background") {
        headless_app app(im_vec2(12, 6));
        set_style(im_role::text, {.fg = {}, .bg = screen_bg, .attrs = 0});
        set_view_style(im_border::rounded, im_border::rounded, im_align::left);
        set_view_look(panel());
        app.frame([] {
            view_begin("one");
            label("a");
            view_end();
        });
        CHECK(app.line(0) == "┃");
        CHECK(app.line(1) == "┃ one");
        CHECK(app.line(2) == "┃ a");
        CHECK(app.line(3) == "┃");
        CHECK(app.line(4) == "");
        CHECK(app.fg(0, 1) == test_active); // active: border_focused
        // the panel is filled, widgets inside inherit its background
        CHECK(app.backend().cell(11, 0).style.bg == panel_bg.value);
        CHECK(app.backend().cell(2, 2).style.bg == panel_bg.value);
        CHECK(app.backend().cell(11, 3).style.bg == panel_bg.value);
        CHECK(app.backend().cell(0, 4).style.bg == screen_bg.value);
    }

    TEST_CASE("bars only on the active view") {
        headless_app app(im_vec2(10, 4));
        set_view_look({.border = im_border::none, .bg = {}, .bar_left = true, .bars_active_only = true});
        app.frame([] {
            view_begin("one", 0);
            label("a");
            view_end();
            view_begin("two", 0);
            label("b");
            view_end();
        });
        CHECK(app.line(0) == "┃a");
        CHECK(app.line(1) == " b");
    }

    TEST_CASE("next look applies once, fixed height is filled") {
        headless_app app(im_vec2(10, 6));
        app.frame([] {
            set_next_view_look({.border = im_border::none, .bg = panel_bg, .pad_x = 2});
            view_begin("one", 0, im_key_id(), 3);
            label("a");
            view_end();
            view_begin("two");
            label("b");
            view_end();
        });
        CHECK(app.line(0) == "  a");
        CHECK(app.backend().cell(9, 2).style.bg == panel_bg.value);
        CHECK(app.line(4) == "│b       │"); // default look: framed
    }

    TEST_CASE("frameless view scrolls with a thumb in the right padding") {
        headless_app app(im_vec2(10, 4));
        set_view_look({.border = im_border::none, .bg = {}, .pad_x = 1});
        app.frame([] {
            view_begin("v", 0, im_key_id(), 4);
            for (auto const* text : {"1", "2", "3", "4", "5", "6", "7", "8"}) {
                label(text);
            }
            view_end();
        });
        app.frame([] {
            view_begin("v", 0, im_key_id(), 4);
            for (auto const* text : {"1", "2", "3", "4", "5", "6", "7", "8"}) {
                label(text);
            }
            view_end();
        });
        CHECK(app.line(0) == " 1       ┃");
    }

    TEST_CASE("frameless popup: title on the left, esc on the right, backdrop") {
        headless_app app(im_vec2(20, 7));
        set_style(im_role::text, {.fg = {}, .bg = screen_bg, .attrs = 0});
        set_popup_look(
            {.border = im_border::none, .bg = panel_bg, .pad_x = 2, .pad_y = 1, .title_gap = 1, .backdrop = 0.5f});
        auto const ui = [] {
            label("under");
            if (popup_begin("p", "Title", 14)) {
                label("x");
                popup_end();
            }
        };
        app.frame([&] {
            open_popup("p");
            ui();
        });
        app.frame(ui);
        // 1 + title + gap + content + 1 = 5 rows, centered in 7
        CHECK(app.line(1) == "");
        CHECK(app.line(2) == "     Title  esc");
        CHECK(app.line(4) == "     x");
        CHECK(app.backend().cell(3, 1).style.bg == panel_bg.value);
        CHECK(app.backend().cell(3, 5).style.bg == panel_bg.value);
        // the rest of the screen is darker
        CHECK(app.backend().cell(0, 6).style.bg == 0x050505u);
        CHECK(app.backend().cell(0, 0).style.bg == 0x050505u);
        CHECK((app.backend().cell(0, 0).style.attrs & im_attr_dim) != 0); // default fg: dimmed
    }

    TEST_CASE("popup without backdrop leaves the screen alone") {
        headless_app app(im_vec2(20, 7));
        set_style(im_role::text, {.fg = {}, .bg = screen_bg, .attrs = 0});
        auto const ui = [] {
            if (popup_begin("p", "t", 10)) {
                label("x");
                popup_end();
            }
        };
        app.frame([&] {
            open_popup("p");
            ui();
        });
        app.frame(ui);
        CHECK(app.backend().cell(0, 0).style.bg == screen_bg.value);
    }
}

TEST_SUITE("widget options for panels") {

    TEST_CASE("text input without prompt") {
        headless_app app(im_vec2(10, 1));
        auto text = std::string("abc");
        app.frame([&] {
            view_begin("v", 0);
            label("-");
            same_line();
            text_input("##t", text, im_input_flag_no_prompt);
            view_end();
        });
        CHECK(app.line(0) == "- abc"); // same_line gap, no "> "
    }

    TEST_CASE("table: no header, row padding, column role") {
        headless_app app(im_vec2(12, 2));
        set_style(im_role::muted, {.fg = im_color(0x555555u), .bg = {}, .attrs = 0});
        static constexpr auto columns =
            std::to_array<im_table_column>({{"name", fill()}, {"dbm", cells(3), im_align::right, im_role::muted}});
        static constexpr auto cells_text = std::to_array({"one"sv, "-40"sv, "two"sv, "-70"sv});
        int selected = 0;
        app.frame([&] {
            view_begin("v", 0);
            table("t", columns, cells_text, selected, 0, {.header = false, .row_pad = 1});
            view_end();
        });
        CHECK(app.line(0) == " one    -40");
        CHECK(app.line(1) == " two    -70");
        CHECK((app.backend().cell(0, 0).style.attrs & im_attr_reverse) != 0); // padding highlighted too
        CHECK(app.backend().cell(8, 0).style.fg != 0x555555u);                // selected row keeps its style
        CHECK(app.backend().cell(8, 1).style.fg == 0x555555u);
        CHECK(app.backend().cell(1, 1).style.fg != 0x555555u);
    }

    TEST_CASE("left aligned label parts take set_next_item_width") {
        headless_app app(im_vec2(10, 1));
        app.frame([] {
            auto const s = scoped_style(im_role::text, {.fg = {}, .bg = panel_bg, .attrs = 0});
            set_next_item_width(fill());
            label({{"ab"}});
        });
        CHECK(app.line(0) == "ab");
        CHECK(app.backend().cell(9, 0).style.bg == panel_bg.value);
    }
}

} // namespace xxx::testing

namespace xxx::testing {

TEST_SUITE("overlay") {

    TEST_CASE("toast in the corner: above views, below popups") {
        headless_app app(im_vec2(20, 6));
        auto const look = im_view_look{.border = {}, .bg = im_color(0x1e1e1eu), .pad_x = 1, .bar_left = true};
        auto const ui = [&] {
            view_begin("v", 0);
            label("aaaaaaaaaaaaaaaaaaaa");
            label("bbbbbbbbbbbbbbbbbbbb");
            view_end();
            overlay_begin(im_vec2(10, 0), 8, look);
            label("saved");
            overlay_end();
            if (popup_begin("p", "p", 6)) {
                label("x");
                popup_end();
            }
        };
        app.frame(ui);
        CHECK(app.line(0) == "aaaaaaaaaa┃ saved aa");
        CHECK(app.line(1) == "bbbbbbbbbbbbbbbbbbbb");
        CHECK(app.backend().cell(17, 0).style.bg == 0x1e1e1eu);
        app.frame([&] {
            open_popup("p");
            ui();
        });
        app.frame(ui);
        CHECK(app.line(1) == "bbbbbbb╭ p ─╮bbbbbbb");
    }
}

} // namespace xxx::testing
