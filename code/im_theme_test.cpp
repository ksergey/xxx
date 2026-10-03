// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "im_renderer.h"
#include "im_theme.h"
#include "test_utils.h"

namespace xxx::testing {

TEST_SUITE("im_theme") {

  TEST_CASE("set default color") {
    im_theme t;
    t.set_default_color(im_color_id::text, 0x112233_c);
    CHECK(t.get_color(im_color_id::text) == 0x112233_c);
  }

  TEST_CASE("push / pop restores previous color") {
    im_theme t;
    t.set_default_color(im_color_id::text, 0x111111_c);
    t.push_color(im_color_id::text, 0x222222_c);
    CHECK(t.get_color(im_color_id::text) == 0x222222_c);
    t.pop_color();
    CHECK(t.get_color(im_color_id::text) == 0x111111_c);
  }

  TEST_CASE("nested push of same id unwinds in order") {
    im_theme t;
    t.set_default_color(im_color_id::border, 0x000001_c);
    t.push_color(im_color_id::border, 0x000002_c);
    t.push_color(im_color_id::border, 0x000003_c);
    t.pop_color();
    CHECK(t.get_color(im_color_id::border) == 0x000002_c);
    t.pop_color();
    CHECK(t.get_color(im_color_id::border) == 0x000001_c);
  }

  TEST_CASE("pop with count") {
    im_theme t;
    t.set_default_color(im_color_id::text, 0x000001_c);
    t.set_default_color(im_color_id::background, 0x000002_c);
    t.push_color(im_color_id::text, 0xaaaaaa_c);
    t.push_color(im_color_id::background, 0xbbbbbb_c);
    t.pop_color(2);
    CHECK(t.get_color(im_color_id::text) == 0x000001_c);
    CHECK(t.get_color(im_color_id::background) == 0x000002_c);
  }

  TEST_CASE("pop more than pushed is safe") {
    im_theme t;
    t.set_default_color(im_color_id::text, 0x000001_c);
    t.push_color(im_color_id::text, 0xaaaaaa_c);
    t.pop_color(10);
    CHECK(t.get_color(im_color_id::text) == 0x000001_c);
  }

  TEST_CASE("reset unwinds everything") {
    im_theme t;
    t.set_default_color(im_color_id::text, 0x000001_c);
    for (int i = 0; i < 5; ++i) {
      t.push_color(im_color_id::text, im_color(std::uint32_t(i + 100)));
    }
    t.reset();
    CHECK(t.get_color(im_color_id::text) == 0x000001_c);
  }

  TEST_CASE("get_style combines fg and bg") {
    im_theme t;
    t.set_default_color(im_color_id::text, 0x123456_c);
    t.set_default_color(im_color_id::background, 0x654321_c);
    auto const s = t.get_style(im_color_id::text, im_color_id::background);
    CHECK(s.fg == 0x123456u);
    CHECK(s.bg == 0x654321u);
  }
}

} // namespace xxx::testing
