// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "im_theme.h"
#include "test_utils.h"

namespace xxx::testing {

TEST_SUITE("im_theme") {

  TEST_CASE("default theme uses only the terminal palette and attributes") {
    im_theme t;
    for (int r = 0; r < int(im_role::last); ++r) {
      auto const s = t.resolved(im_role(r));
      CAPTURE(r);
      CHECK_FALSE(s.fg->is_rgb()); // no fixed colors: adapts to light and dark schemes
      CHECK_FALSE(s.bg->is_rgb());
    }
    CHECK(t.get(im_role::focus).attrs == im_attr_reverse);
    CHECK(t.get(im_role::selection).attrs == im_attr_underline);
  }

  TEST_CASE("unset colors come from text") {
    im_theme t;
    t.set(im_role::text, {.fg = im_color(0x111111u), .bg = im_color(0x222222u), .attrs = 0});
    t.set(im_role::error, {.fg = ansi::red, .bg = {}, .attrs = im_attr_bold});
    auto const s = t.style(im_role::error);
    CHECK(s.fg == ansi::red.value);
    CHECK(s.bg == 0x222222u); // theme background applies everywhere
    CHECK(s.attrs == im_attr_bold);
  }

  TEST_CASE("custom style resolves the same way") {
    im_theme t;
    t.set(im_role::text, {.fg = im_color(0x111111u), .bg = im_color(0x222222u), .attrs = 0});
    auto const s = t.style(im_role_style{.fg = ansi::green, .bg = {}, .attrs = 0});
    CHECK(s.fg == ansi::green.value);
    CHECK(s.bg == 0x222222u);
  }

  TEST_CASE("push / pop restore, nested") {
    im_theme t;
    auto const before = t.style(im_role::text);
    t.push(im_role::text, {.fg = ansi::red, .bg = {}, .attrs = 0});
    t.push(im_role::text, {.fg = ansi::blue, .bg = {}, .attrs = 0});
    CHECK(t.style(im_role::text).fg == ansi::blue.value);
    t.pop();
    CHECK(t.style(im_role::text).fg == ansi::red.value);
    t.pop();
    CHECK(t.style(im_role::text) == before);
  }

  TEST_CASE("pop more than pushed is safe, reset drops overrides") {
    im_theme t;
    auto const before = t.style(im_role::accent);
    t.push(im_role::accent, {.fg = ansi::red, .bg = {}, .attrs = 0});
    t.pop(10);
    CHECK(t.style(im_role::accent) == before);
    t.push(im_role::accent, {.fg = ansi::red, .bg = {}, .attrs = 0});
    t.push(im_role::muted, {.fg = ansi::red, .bg = {}, .attrs = 0});
    t.reset();
    CHECK(t.style(im_role::accent) == before);
  }

  TEST_CASE("presets") {
    im_theme t;
    t.use(im_theme_preset::classic);
    CHECK(t.get(im_role::accent).fg == im_color(0xdcf763u));
    t.use(im_theme_preset::terminal);
    CHECK(t.get(im_role::accent).fg == ansi::cyan);
  }

  TEST_CASE("use drops overrides") {
    im_theme t;
    t.push(im_role::accent, {.fg = ansi::red, .bg = {}, .attrs = 0});
    t.use(im_theme_preset::terminal);
    t.pop(); // nothing left to pop
    CHECK(t.get(im_role::accent).fg == ansi::cyan);
  }
}

} // namespace xxx::testing
