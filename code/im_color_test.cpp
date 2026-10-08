// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "test_utils.h"

namespace xxx::testing {

TEST_SUITE("im_color") {

    TEST_CASE("default is the terminal color, not black") {
        CHECK(im_color().is_default());
        CHECK(im_color() == im_color::terminal_default());
        CHECK_FALSE(im_color(0x000000u).is_default()); // black is a real color now
        CHECK(im_color(0x000000u).is_rgb());
    }

    TEST_CASE("kinds") {
        static_assert(ansi::red.is_indexed() && ansi::red.index() == 1);
        static_assert(ansi::bright_white.index() == 15);
        static_assert(im_color::indexed(200).index() == 200);
        static_assert(im_color::rgb(1, 2, 3).value == 0x010203u);
        static_assert((0xdcf763_c).is_rgb());
        static_assert(!ansi::red.is_rgb() && !ansi::red.is_default());
        CHECK(true);
    }

    TEST_CASE("literal keeps 24 bits") {
        static_assert((0x1ff0000_c).value == 0xff0000u);
        CHECK(true);
    }

    TEST_CASE("from floats") {
        CHECK(im_color(1.0f, 0.0f, 0.0f) == im_color(0xff0000u));
        CHECK(im_color(0.0f, 1.0f, 0.0f) == im_color(0x00ff00u));
        CHECK(im_color(0.0f, 0.0f, 1.0f) == im_color(0x0000ffu));
        CHECK(im_color(1.0f, 1.0f, 1.0f) == im_color(0xffffffu));
    }

    TEST_CASE("literal") {
        constexpr auto c = 0x123456_c;
        static_assert(c.value == 0x123456u);
        CHECK(std::uint32_t(c) == 0x123456u);
    }

    TEST_CASE("comparison") {
        CHECK(0xabcdef_c == im_color(0xabcdefu));
        CHECK(0x000001_c < 0x000002_c);
    }
}

} // namespace xxx::testing
