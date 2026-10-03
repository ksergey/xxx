// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "test_utils.h"

namespace xxx::testing {

TEST_SUITE("im_vec2") {

  TEST_CASE("default is zero") {
    CHECK(im_vec2() == im_vec2(0, 0));
  }

  TEST_CASE("arithmetic") {
    CHECK(im_vec2(1, 2) + im_vec2(3, 4) == im_vec2(4, 6));
    CHECK(im_vec2(1, 2) - im_vec2(3, 5) == im_vec2(-2, -3));
    CHECK(-im_vec2(1, -2) == im_vec2(-1, 2));
  }

  TEST_CASE("compound assignment") {
    auto v = im_vec2(1, 1);
    v += im_vec2(2, 3);
    CHECK(v == im_vec2(3, 4));
    v -= im_vec2(1, 1);
    CHECK(v == im_vec2(2, 3));
  }

  TEST_CASE("ordering is lexicographic") {
    CHECK(im_vec2(0, 9) < im_vec2(1, 0));
    CHECK(im_vec2(1, 0) < im_vec2(1, 1));
  }

  TEST_CASE("format") {
    CHECK(std::format("{}", im_vec2(-1, 7)) == "im_vec2(-1, 7)");
  }
}

} // namespace xxx::testing
