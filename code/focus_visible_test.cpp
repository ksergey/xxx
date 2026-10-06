// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <array>
#include <string>
#include <string_view>

#include "test_headless.h"

namespace xxx::testing {

using namespace std::string_view_literals;

namespace {

// all theme colors the same: focus must still be visible
void monochrome() {
  for (int r = 0; r < int(im_role::last); ++r) {
    auto const role = im_role(r);
    set_style(role, {.fg = im_color(0xc0c0c0u), .bg = im_color(0xc0c0c0u), .attrs = get_style(role).attrs});
  }
}

struct widgets_app : headless_app {
  bool flag = true;
  int tab = 0;
  int row = 1;

  widgets_app() : headless_app(im_vec2(30, 8)) {
    monochrome();
    step();
  }
  void step() {
    static constexpr auto items = std::to_array({"one"sv, "two"sv, "three"sv});
    frame([this] {
      view_begin("v", 0);
      button("ok");                // row 0
      checkbox("flag", flag);      // row 1
      tabs("t", items, tab);       // row 2
      list("l", items, row, 3);    // rows 3..5
      view_end();
    });
  }
  void tab_key() {
    backend().push_key(im_key_id::tab);
    step();
  }
  auto reversed(int x0, int x1, int y) const -> bool {
    for (int x = x0; x <= x1; ++x) {
      if (!(backend_cell(x, y).style.attrs & im_attr_reverse)) {
        return false;
      }
    }
    return true;
  }
  auto underlined(int x, int y) const -> bool {
    return backend_cell(x, y).style.attrs & im_attr_underline;
  }
  auto backend_cell(int x, int y) const -> im_cell const& {
    return const_cast<widgets_app*>(this)->backend().cell(x, y);
  }
};

} // namespace

TEST_SUITE("visible focus") {

  TEST_CASE("only the focused widget is reversed, in a monochrome theme") {
    widgets_app app;
    // button "  [ ok ]": " ok " between the brackets is reversed
    CHECK(app.line(0) == "  [ ok ]");
    CHECK(app.reversed(3, 6, 0));
    CHECK_FALSE(app.reversed(2, 2, 0)); // brackets stay as they are
    CHECK_FALSE(app.reversed_in_row(1));
    CHECK_FALSE(app.reversed_in_row(2));
    CHECK_FALSE(app.reversed_in_row(4));
  }

  TEST_CASE("checkbox: the box") {
    widgets_app app;
    app.tab_key();
    CHECK(app.reversed(0, 2, 1));
    CHECK_FALSE(app.reversed(4, 7, 1)); // label
    CHECK_FALSE(app.reversed_in_row(0));
  }

  TEST_CASE("tabs: selected tab reversed with focus, underlined without") {
    widgets_app app;
    CHECK(app.underlined(1, 2));
    CHECK_FALSE(app.reversed_in_row(2));
    app.tab_key();
    app.tab_key();
    CHECK(app.reversed(0, 4, 2)); // " one "
    CHECK_FALSE(app.underlined(1, 2));
  }

  TEST_CASE("list: selected row reversed with focus, underlined without") {
    widgets_app app;
    CHECK(app.underlined(0, 4));
    CHECK_FALSE(app.reversed_in_row(4));
    for (int i = 0; i < 3; ++i) {
      app.tab_key();
    }
    CHECK(app.reversed(0, 29, 4));
    CHECK_FALSE(app.reversed_in_row(0));
  }

  TEST_CASE("exactly one widget shows focus at a time") {
    widgets_app app;
    for (int i = 0; i < 4; ++i) {
      auto rows = 0;
      for (int y = 0; y < 6; ++y) {
        rows += app.reversed_in_row(y) ? 1 : 0;
      }
      CHECK(rows == 1);
      app.tab_key();
    }
  }
}

} // namespace xxx::testing
