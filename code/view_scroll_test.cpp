// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <format>
#include <string>

#include "test_headless.h"

namespace xxx::testing {

namespace {

// view with `rows` labels "line N", fixed height 6 (4 visible content rows)
auto log_view(int rows, int height = 6) {
  return [=] {
    view_begin("log", im_view_flags_default, {}, height);
    for (int i = 0; i < rows; ++i) {
      label(std::format("line {}", i));
    }
    view_end();
    label("footer");
  };
}

} // namespace

TEST_SUITE("view scroll") {

  TEST_CASE("fixed height is kept when content is shorter") {
    headless_app app(im_vec2(16, 8));
    app.frame(log_view(2));
    CHECK(app.screen() == dedent(R"(
      ╭──── log ─────╮
      │line 0        │
      │line 1        │
      │              │
      │              │
      ╰──────────────╯
      footer
    )"));
  }

  TEST_CASE("content is clipped by viewport, scrollbar shown") {
    headless_app app(im_vec2(16, 8));
    app.frame(log_view(10));
    CHECK(app.screen() == dedent(R"(
      ╭──── log ─────╮
      │line 0        ┃
      │line 1        │
      │line 2        │
      │line 3        │
      ╰──────────────╯
      footer
    )"));
  }

  TEST_CASE("page down / page up keep one line of context and clamp") {
    headless_app app(im_vec2(16, 8));
    auto const ui = log_view(10);
    app.frame(ui);

    app.backend().push_key(im_key_id::page_down);
    app.frame(ui);
    CHECK(app.line(1) == "│line 3        │");

    app.backend().push_key(im_key_id::page_down);
    app.frame(ui);
    CHECK(app.screen() == dedent(R"(
      ╭──── log ─────╮
      │line 6        │
      │line 7        │
      │line 8        │
      │line 9        ┃
      ╰──────────────╯
      footer
    )"));

    app.backend().push_key(im_key_id::page_down); // already at bottom
    app.frame(ui);
    CHECK(app.line(1) == "│line 6        │");

    app.backend().push_key(im_key_id::page_up);
    app.frame(ui);
    CHECK(app.line(1) == "│line 3        │");
    app.backend().push_key(im_key_id::page_up);
    app.frame(ui);
    CHECK(app.line(1) == "│line 0        ┃");
  }

  TEST_CASE("no scrolling when content fits") {
    headless_app app(im_vec2(16, 8));
    auto const ui = log_view(4);
    app.frame(ui);
    app.backend().push_key(im_key_id::page_down);
    app.frame(ui);
    CHECK(app.line(1) == "│line 0        │"); // no scrollbar either
  }

  TEST_CASE("mouse wheel scrolls hovered view by 3 rows") {
    headless_app app(im_vec2(16, 8));
    auto const ui = log_view(10);
    app.frame(ui);

    app.backend().push_mouse_wheel(+1, im_vec2(4, 2));
    app.frame(ui);
    CHECK(app.line(1) == "│line 3        │");

    SUBCASE("on border counts as hover") {
      app.backend().push_mouse_wheel(-1, im_vec2(0, 0));
      app.frame(ui);
      CHECK(app.line(1) == "│line 0        ┃");
    }
    SUBCASE("outside view is ignored") {
      app.backend().push_mouse_wheel(-1, im_vec2(3, 6));
      app.frame(ui);
      CHECK(app.line(1) == "│line 3        │");
    }
  }

  TEST_CASE("scrollbar thumb size and position") {
    headless_app app(im_vec2(10, 7));
    // viewport 4 rows, content 8 rows -> thumb 2 rows
    auto const ui = [] {
      view_begin("v", im_view_flags_default, {}, 6);
      for (int i = 0; i < 8; ++i) {
        label(std::format("{}", i));
      }
      view_end();
    };
    auto const thumb_rows = [&] {
      auto rows = std::string();
      for (int y = 1; y <= 4; ++y) {
        rows += app.backend().cell(9, y).ch == U'┃' ? '#' : '.';
      }
      return rows;
    };
    app.frame(ui);
    CHECK(thumb_rows() == "##..");
    app.backend().push_mouse_wheel(+1, im_vec2(1, 1)); // offset 3 of 4
    app.frame(ui);
    CHECK(thumb_rows() == ".##.");
    app.backend().push_key(im_key_id::page_down);
    app.frame(ui);
    CHECK(thumb_rows() == "..##");
  }

  TEST_CASE("page keys scroll only active view, wheel any hovered") {
    headless_app app(im_vec2(16, 14));
    auto const ui = [] {
      for (auto const name : {"a", "b"}) {
        view_begin(name, im_view_flags_default, {}, 5);
        for (int i = 0; i < 6; ++i) {
          label(std::format("{} {}", name, i));
        }
        view_end();
      }
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::page_down);
    app.frame(ui);
    CHECK(app.line(1) == "│a 2           │");
    CHECK(app.line(6) == "│b 0           ┃");

    app.backend().push_mouse_wheel(+1, im_vec2(5, 7));
    app.frame(ui);
    CHECK(app.line(1) == "│a 2           │"); // state is per view
    CHECK(app.line(6) == "│b 3           │");
  }

  TEST_CASE("offset is clamped when content shrinks") {
    headless_app app(im_vec2(16, 8));
    int rows = 10;
    auto const ui = [&] { log_view(rows)(); };
    app.frame(ui);
    for (int i = 0; i < 3; ++i) {
      app.backend().push_key(im_key_id::page_down);
      app.frame(ui);
    }
    REQUIRE(app.line(1) == "│line 6        │");

    rows = 6;
    app.frame(ui); // drawn with stale offset, then clamped
    app.frame(ui);
    CHECK(app.line(1) == "│line 2        │");
    CHECK(app.line(4) == "│line 5        ┃");
  }

  TEST_CASE("without border scrolls but has no scrollbar") {
    headless_app app(im_vec2(10, 4));
    auto const ui = [] {
      view_begin("v", 0, {}, 2);
      for (int i = 0; i < 5; ++i) {
        label(std::format("row {}", i));
      }
      view_end();
      label("end");
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::page_down);
    app.frame(ui);
    CHECK(app.screen() == "row 1\nrow 2\nend");
  }

  TEST_CASE("auto height view ignores page keys") {
    headless_app app(im_vec2(16, 8));
    auto const ui = [] {
      view_begin("v");
      label("a");
      label("b");
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::page_down);
    app.frame(ui);
    CHECK(app.line(1) == "│a             │");
  }
}

TEST_SUITE("view fill height") {

  TEST_CASE("fill stretches to screen bottom") {
    headless_app app(im_vec2(10, 5));
    app.frame([] {
      view_begin("v", im_view_flags_default, {}, fill());
      label("x");
      view_end();
    });
    CHECK(app.screen() == dedent(R"(
      ╭── v ───╮
      │x       │
      │        │
      │        │
      ╰────────╯
    )"));
  }

  TEST_CASE("fill(n) leaves rows for footer") {
    headless_app app(im_vec2(10, 6));
    app.frame([] {
      label("header");
      view_begin("v", im_view_flags_default, {}, fill(1));
      view_end();
      label("footer");
    });
    CHECK(app.screen() == dedent(R"(
      header
      ╭── v ───╮
      │        │
      │        │
      ╰────────╯
      footer
    )"));
  }

  TEST_CASE("fill inside row columns") {
    headless_app app(im_vec2(20, 4));
    app.frame([] {
      layout_row_begin(2);
      layout_row_push(0.5);
      view_begin("a", im_view_flags_default, {}, fill());
      view_end();
      layout_row_push(0.5);
      view_begin("b", im_view_flags_default, {}, fill());
      view_end();
      layout_row_end();
    });
    CHECK(app.screen() == dedent(R"(
      ╭── a ───╮╭── b ───╮
      │        ││        │
      │        ││        │
      ╰────────╯╰────────╯
    )"));
  }

  TEST_CASE("nested in fixed height view fills that view") {
    headless_app app(im_vec2(12, 8));
    app.frame([] {
      view_begin("outer", im_view_flags_default, {}, 5);
      panel_begin();
      panel_end();
      view_end();
      label("below");
    });
    CHECK(app.line(5) == "below");
  }
}

TEST_SUITE("view scroll focus") {

  struct buttons_app : headless_app {
    int pressed = -1;

    buttons_app() : headless_app(im_vec2(16, 6)) {}

    void step() {
      frame([this] {
        pressed = -1;
        // 2 visible rows, 6 buttons
        view_begin("v", im_view_flags_default, {}, 4);
        for (int i = 0; i < 6; ++i) {
          if (button(std::format("b{}", i))) {
            pressed = i;
          }
        }
        view_end();
      });
    }
    void key(im_key_id k, int times = 1) {
      for (int i = 0; i < times; ++i) {
        backend().push_key(k);
        step();
      }
    }
    auto first_visible() const -> std::string {
      return line(1).substr(std::string_view("│").size());
    }
  };

  TEST_CASE("tab scrolls focused widget into view") {
    buttons_app app;
    app.step();
    CHECK(app.first_visible().starts_with("  [ b0 ]"));

    app.key(im_key_id::tab, 3); // focus b3, below viewport
    app.step();                 // scroll is applied on next frame
    CHECK(app.line(2).find("[ b3 ]") != std::string::npos);

    app.backend().push_key(im_key_id::enter);
    app.step();
    CHECK(app.pressed == 3);
  }

  TEST_CASE("wrapping focus scrolls back to top") {
    buttons_app app;
    app.step();
    app.key(im_key_id::tab, 6); // b5 -> wraps to b0
    app.step();
    CHECK(app.first_visible().starts_with("  [ b0 ]"));
  }

  TEST_CASE("manual scroll is not overridden while focus stays") {
    buttons_app app;
    app.step();
    app.key(im_key_id::page_down, 2);
    app.step();
    CHECK(app.line(1).find("[ b0 ]") == std::string::npos);
  }
}

} // namespace xxx::testing
