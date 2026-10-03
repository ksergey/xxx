// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "test_utils.h"

namespace xxx::testing {

TEST_SUITE("im_rect") {

  TEST_CASE("default is empty") {
    CHECK(im_rect().empty());
    CHECK_FALSE(im_rect());
  }

  TEST_CASE("single cell") {
    auto const r = im_rect(3, 4, 3, 4);
    CHECK(r.valid());
    CHECK(r.size() == im_vec2(1, 1));
  }

  TEST_CASE("corners") {
    auto const r = im_rect(1, 2, 5, 7);
    CHECK(r.top_left() == im_vec2(1, 2));
    CHECK(r.top_right() == im_vec2(5, 2));
    CHECK(r.bottom_left() == im_vec2(1, 7));
    CHECK(r.bottom_right() == im_vec2(5, 7));
  }

  TEST_CASE("set_size") {
    auto r = im_rect(2, 2, 2, 2);
    r.set_size(im_vec2(4, 3));
    CHECK(r == im_rect(2, 2, 5, 4));
    r.set_width(1);
    CHECK(r.width() == 1);
    r.set_height(0);
    CHECK(r.empty());
  }

  TEST_CASE("translate") {
    CHECK(im_rect(0, 0, 2, 2).translate(im_vec2(3, -1)) == im_rect(3, -1, 5, 1));
  }

  TEST_CASE("crop") {
    auto const r = im_rect(0, 0, 9, 9);
    CHECK(r.crop(1) == im_rect(1, 1, 8, 8));
    CHECK(r.crop(1, 2, 3, 4) == im_rect(1, 2, 6, 5));
    CHECK(r.crop_left(2) == im_rect(2, 0, 9, 9));
    CHECK(r.crop_top(2) == im_rect(0, 2, 9, 9));
    CHECK(r.crop_right(2) == im_rect(0, 0, 7, 9));
    CHECK(r.crop_bottom(2) == im_rect(0, 0, 9, 7));
    CHECK(r.crop(5).empty());
  }

  TEST_CASE("intersection") {
    SUBCASE("commutative") {
      auto const a = im_rect(0, 0, 10, 5);
      auto const b = im_rect(4, -2, 20, 3);
      CHECK(a.intersection(b) == b.intersection(a));
      CHECK(a.intersection(b) == im_rect(4, 0, 10, 3));
    }
    SUBCASE("with invalid is empty") {
      CHECK(im_rect(0, 0, 5, 5).intersection(im_rect()).empty());
      CHECK(im_rect().intersection(im_rect(0, 0, 5, 5)).empty());
    }
    SUBCASE("touching edge") {
      CHECK(im_rect(0, 0, 4, 4).intersection(im_rect(4, 4, 8, 8)) == im_rect(4, 4, 4, 4));
      CHECK(im_rect(0, 0, 4, 4).intersection(im_rect(5, 0, 8, 4)).empty());
    }
  }

  TEST_CASE("format") {
    CHECK(std::format("{}", im_rect(1, 2, 3, 4)) == "im_rect(1, 2, 3, 4)");
    // invalid rects get "<!>" prefix
    CHECK(std::format("{}", im_rect()) == "<!>im_rect()");
    CHECK(std::format("{}", im_rect(2, 3, 0, 1)) == "<!>im_rect(2, 3, 0, 1)");
  }
}

} // namespace xxx::testing
