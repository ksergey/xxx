// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string_view>
#include <vector>

#include <doctest/doctest.h>

#include "ansi_input.h"
#include "test_utils.h"

namespace xxx::testing {

TEST_SUITE("im_event stream") {

    TEST_CASE("parser emits events in order with their fields") {
        auto parser = ansi_input_parser();
        auto events = std::vector<im_event>();
        parser.feed("a\x1b[1;5C\x1b[<0;3;2M\x1b[<65;3;2M\x1bx", events);
        REQUIRE(events.size() == 6);
        CHECK(events[0].kind == im_event::type::text);
        CHECK(events[0].ch == 'a');
        CHECK(events[1].kind == im_event::type::key);
        CHECK(events[1].key == im_key_id::arrow_right);
        CHECK(events[1].mods == im_mod_ctrl);
        CHECK(events[2].kind == im_event::type::mouse_move); // press comes with its position
        CHECK(events[3].kind == im_event::type::mouse_press);
        CHECK(events[3].button == im_mouse_button_id::left);
        CHECK(events[3].pos == im_vec2(2, 1));
        CHECK(events[4].kind == im_event::type::mouse_wheel);
        CHECK(events[4].wheel == 1);
        CHECK(events[5].kind == im_event::type::text);
        CHECK(events[5].mods == im_mod_alt);
    }

    TEST_CASE("space is text and a key") {
        auto parser = ansi_input_parser();
        auto events = std::vector<im_event>();
        parser.feed(" ", events);
        REQUIRE(events.size() == 2);
        CHECK(events[0].kind == im_event::type::text);
        CHECK(events[1].key == im_key_id::space);
    }

    TEST_CASE("lone esc is resolved by flush") {
        auto parser = ansi_input_parser();
        auto events = std::vector<im_event>();
        parser.feed("\x1b", events);
        CHECK(events.empty());
        CHECK(parser.pending());
        parser.flush(events);
        REQUIRE(events.size() == 1);
        CHECK(events[0].key == im_key_id::esc);
    }

    TEST_CASE("apply_event fills the frame input state") {
        auto input = im_input();
        input.reset();
        apply_event({.kind = im_event::type::key, .key = im_key_id::enter}, input);
        apply_event({.kind = im_event::type::text, .ch = 'q'}, input);
        apply_event(
            {.kind = im_event::type::mouse_press, .button = im_mouse_button_id::right, .pos = im_vec2(4, 5)}, input);
        apply_event({.kind = im_event::type::mouse_wheel, .pos = im_vec2(1, 1), .wheel = -2}, input);
        apply_event({.kind = im_event::type::resize, .pos = im_vec2(80, 24)}, input); // nothing to store
        CHECK(input.is_key_pressed(im_key_id::enter));
        CHECK(input.get_input_events().size() == 2);
        CHECK(input.is_mouse_clicked(im_mouse_button_id::right));
        CHECK(input.mouse_clicked_pos(im_mouse_button_id::right) == im_vec2(4, 5));
        CHECK(input.mouse_wheel() == -2);
    }
}

} // namespace xxx::testing
