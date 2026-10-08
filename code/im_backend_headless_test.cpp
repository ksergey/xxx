// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "test_headless.h"

namespace xxx::testing {

TEST_SUITE("im_backend_headless") {

    TEST_CASE("blank screen") {
        im_backend_headless b(im_vec2(5, 3));
        CHECK(b.size() == im_vec2(5, 3));
        CHECK(b.screen() == "");
        CHECK(b.frames() == 0);
    }

    TEST_CASE("cells are visible only after present") {
        im_backend_headless b(im_vec2(5, 2));
        b.clear({});
        b.set_cell(1, 0, 'x', {});
        CHECK(b.line(0) == "");
        b.present();
        CHECK(b.line(0) == " x");
        CHECK(b.frames() == 1);
    }

    TEST_CASE("clear fills with spaces and style") {
        im_backend_headless b(im_vec2(3, 1));
        auto const style = im_style(im_color(0x112233u), im_color(0x445566u));
        b.set_cell(0, 0, 'x', {});
        b.clear(style);
        b.present();
        CHECK(b.cell(0, 0).ch == ' ');
        CHECK(b.cell(2, 0).style.bg == 0x445566u);
    }

    TEST_CASE("out of screen cells are ignored") {
        im_backend_headless b(im_vec2(2, 2));
        b.clear({});
        b.set_cell(-1, 0, 'x', {});
        b.set_cell(2, 0, 'x', {});
        b.set_cell(0, 2, 'x', {});
        b.present();
        CHECK(b.screen() == "");
    }

    TEST_CASE("wide char covers next cell, like a terminal") {
        im_backend_headless b(im_vec2(4, 1));
        b.clear({});
        b.set_cell(0, 0, U'日', {});
        b.set_cell(1, 0, 'x', {}); // hidden under wide char
        b.set_cell(2, 0, 'y', {});
        b.present();
        CHECK(b.line(0) == "日y");
    }

    TEST_CASE("screen joins lines and trims trailing blanks") {
        im_backend_headless b(im_vec2(3, 4));
        b.clear({});
        b.set_cell(0, 0, 'a', {});
        b.set_cell(2, 2, 'c', {});
        b.present();
        CHECK(b.screen() == "a\n\n  c");
    }

    TEST_CASE("events are delivered once") {
        im_backend_headless b(im_vec2(1, 1));
        b.push_key(im_key_id::enter);
        b.push_text("я ");

        im_input in;
        b.poll_events(in, std::chrono::milliseconds(0));
        CHECK(in.is_key_pressed(im_key_id::enter));
        CHECK(in.is_key_pressed(im_key_id::space)); // typed space also is a key
        auto const events = in.get_input_events();
        REQUIRE(events.size() == 4);
        CHECK(events[1].ch == std::uint32_t(U'я'));

        in.reset();
        b.poll_events(in, std::chrono::milliseconds(0));
        CHECK(in.get_input_events().empty());
    }

    TEST_CASE("time advances only on request") {
        im_backend_headless b(im_vec2(1, 1));
        auto const t0 = b.now();
        CHECK(b.now() == t0);
        b.advance_time(std::chrono::milliseconds(250));
        CHECK(b.now() - t0 == std::chrono::milliseconds(250));
    }

    TEST_CASE("resize") {
        im_backend_headless b(im_vec2(2, 2));
        b.resize(im_vec2(7, 3));
        CHECK(b.size() == im_vec2(7, 3));
        b.clear({});
        b.set_cell(6, 2, 'z', {});
        b.present();
        CHECK(b.line(2) == "      z");
    }
}

TEST_SUITE("dedent") {
    TEST_CASE("strips common indent and edges") {
        CHECK(dedent(R"(
      ab
        c

    )") == "ab\n  c");
        CHECK(dedent("x") == "x");
    }
}

} // namespace xxx::testing
