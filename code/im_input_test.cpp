// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include "im_input.h"

namespace xxx::testing {

TEST_SUITE("im_input") {

    TEST_CASE("initially nothing pressed") {
        im_input in;
        CHECK_FALSE(in.is_key_pressed(im_key_id::enter));
        CHECK(in.get_input_events().empty());
    }

    TEST_CASE("key event") {
        im_input in;
        in.add_key_event(im_key_id::enter);
        CHECK(in.is_key_pressed(im_key_id::enter));
        CHECK_FALSE(in.is_key_pressed(im_key_id::esc));

        auto const events = in.get_input_events();
        REQUIRE(events.size() == 1);
        CHECK(events[0].key == im_key_id::enter);
        CHECK(events[0].ch == 0u);
    }

    TEST_CASE("character event") {
        im_input in;
        in.add_character(U'ж');
        auto const events = in.get_input_events();
        REQUIRE(events.size() == 1);
        CHECK(events[0].ch == std::uint32_t(U'ж'));
        CHECK(events[0].key == im_key_id());
    }

    TEST_CASE("events keep order") {
        im_input in;
        in.add_character('a');
        in.add_key_event(im_key_id::backspace);
        in.add_character('b');
        auto const events = in.get_input_events();
        REQUIRE(events.size() == 3);
        CHECK(events[0].ch == std::uint32_t('a'));
        CHECK(events[1].key == im_key_id::backspace);
        CHECK(events[2].ch == std::uint32_t('b'));
    }

    TEST_CASE("utf8 characters") {
        im_input in;
        in.add_characters_utf8("хэй");
        auto const events = in.get_input_events();
        REQUIRE(events.size() == 3);
        CHECK(events[0].ch == std::uint32_t(U'х'));
        CHECK(events[1].ch == std::uint32_t(U'э'));
        CHECK(events[2].ch == std::uint32_t(U'й'));
    }

    TEST_CASE("event queue is not capped") {
        im_input in;
        for (int i = 0; i < 1000; ++i) {
            in.add_character('x');
        }
        in.add_key_event(im_key_id::enter);
        CHECK(in.get_input_events().size() == 1001);
        CHECK(in.get_input_events().back().key == im_key_id::enter);
    }

    TEST_CASE("key press count") {
        im_input in;
        CHECK(in.key_press_count(im_key_id::arrow_down) == 0);
        in.add_key_event(im_key_id::arrow_down);
        in.add_key_event(im_key_id::arrow_down);
        in.add_key_event(im_key_id::arrow_down);
        CHECK(in.key_press_count(im_key_id::arrow_down) == 3);
        CHECK(in.is_key_pressed(im_key_id::arrow_down));
        in.reset();
        CHECK(in.key_press_count(im_key_id::arrow_down) == 0);
    }

    TEST_CASE("reset clears state") {
        im_input in;
        in.add_key_event(im_key_id::tab);
        in.add_character('q');
        in.reset();
        CHECK_FALSE(in.is_key_pressed(im_key_id::tab));
        CHECK(in.get_input_events().empty());
    }
}

} // namespace xxx::testing
