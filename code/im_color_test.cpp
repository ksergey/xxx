// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "test_utils.h"

namespace xxx::testing {

TEST_SUITE("im_color") {

  TEST_CASE("default is zero") {
    CHECK(im_color().value == 0u);
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
