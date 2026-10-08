// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <array>
#include <string>
#include <string_view>

#include "test_headless.h"

namespace xxx::testing {

using namespace std::string_view_literals;

namespace {

void two_views() {
  view_begin("one");
  label("a");
  view_end();
  view_begin("two");
  label("b");
  view_end();
}

} // namespace

TEST_SUITE("view style") {

  TEST_CASE("default: rounded frames, centered titles") {
    headless_app app(im_vec2(12, 6));
    app.frame(two_views);
    CHECK(app.line(0) == "╭── one ───╮");
    CHECK(app.line(3) == "╭── two ───╮");
  }

  TEST_CASE("thick active, plain inactive, titles on the left") {
    headless_app app(im_vec2(12, 6));
    set_view_style(im_border::plain, im_border::thick, im_align::left);
    app.frame(two_views);
    CHECK(app.line(0) == "┏ one ━━━━━┓"); // the first view is active
    CHECK(app.line(1) == "┃a         ┃");
    CHECK(app.line(2) == "┗━━━━━━━━━━┛");
    CHECK(app.line(3) == "┌ two ─────┐");
    CHECK(app.line(5) == "└──────────┘");
  }

  TEST_CASE("right titles, double frames") {
    headless_app app(im_vec2(12, 3));
    set_view_style(im_border::double_line, im_border::double_line, im_align::right);
    app.frame([] {
      view_begin("one");
      view_end();
    });
    CHECK(app.line(0) == "╔═════ one ╗");
  }

  TEST_CASE("popups use the active frame") {
    headless_app app(im_vec2(20, 8));
    set_view_style(im_border::rounded, im_border::thick);
    auto const ui = [] {
      if (popup_begin("p", "p", 10)) {
        label("x");
        popup_end();
      }
    };
    app.frame([&] {
      open_popup("p");
      ui();
    });
    app.frame(ui);
    CHECK(app.screen().find("┏") != std::string::npos);
    CHECK(app.screen().find("╭") == std::string::npos);
  }
}

TEST_SUITE("label parts") {

  TEST_CASE("parts with their own styles") {
    headless_app app(im_vec2(20, 1));
    app.frame([] { label({{"s", im_role::text, im_attr_bold}, {" Scan | "}, {"q", im_role::error}}); });
    CHECK(app.line(0) == "s Scan | q");
    CHECK((app.backend().cell(0, 0).style.attrs & im_attr_bold) != 0);
    CHECK((app.backend().cell(2, 0).style.attrs & im_attr_bold) == 0);
    CHECK(app.backend().cell(9, 0).style.fg == get_style(im_role::error).fg->value);
  }

  TEST_CASE("centered and right aligned") {
    headless_app app(im_vec2(10, 2));
    app.frame([] {
      label({{"ab"}, {"cd"}}, im_align::center);
      label({{"xy"}}, im_align::right);
    });
    CHECK(app.line(0) == "   abcd");
    CHECK(app.line(1) == "        xy");
  }

  TEST_CASE("wide characters count as two") {
    headless_app app(im_vec2(10, 1));
    app.frame([] { label({{"日本"}}, im_align::right); });
    CHECK(app.line(0) == "      日本");
  }
}

TEST_SUITE("table options") {

  auto const columns = std::to_array<im_table_column>({{"name", fill()}});
  auto const cells = std::to_array({"one"sv, "two"sv, "three"sv});

  TEST_CASE("header gap") {
    headless_app app(im_vec2(10, 6));
    int selected = 0;
    app.frame([&] { table("t", columns, cells, selected, 0, {.row_roles = {}, .header_gap = true}); });
    CHECK(app.line(0) == "name");
    CHECK(app.line(1) == "");
    CHECK(app.line(2) == "one");
    CHECK(app.line(4) == "three");
  }

  TEST_CASE("row roles; the selected row keeps the selection style") {
    headless_app app(im_vec2(10, 4));
    set_style(im_role::muted, {.fg = im_color(0x555555u), .bg = {}, .attrs = 0});
    auto const roles = std::to_array({im_role::text, im_role::muted, im_role::muted});
    int selected = 2;
    app.frame([&] {
      view_begin("v", 0);
      table("t", columns, cells, selected, 0, {.row_roles = roles, .header_gap = false});
      view_end();
    });
    CHECK(app.backend().cell(0, 1).style.fg != 0x555555u);
    CHECK(app.backend().cell(0, 2).style.fg == 0x555555u);
    CHECK((app.backend().cell(0, 3).style.attrs & im_attr_reverse) != 0); // focused, selected
  }

  TEST_CASE("clicks map to rows below the gap") {
    headless_app app(im_vec2(10, 6));
    int selected = 0;
    auto activated = false;
    auto const ui = [&] {
      view_begin("v", 0);
      activated = table("t", columns, cells, selected, 0, {.row_roles = {}, .header_gap = true});
      view_end();
    };
    app.frame(ui);
    app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(1, 3));
    app.frame(ui);
    CHECK(activated);
    CHECK(selected == 1);
  }
}

TEST_SUITE("vim keys") {

  struct list_app : headless_app {
    int selected = 0;
    list_app() : headless_app(im_vec2(10, 5)) {
      step();
    }
    void step() {
      static constexpr auto items = std::to_array({"a"sv, "b"sv, "c"sv, "d"sv});
      frame([this] {
        view_begin("v", 0);
        list("l", items, selected);
        view_end();
      });
    }
    void type(std::string_view s) {
      backend().push_text(s);
      step();
    }
  };

  TEST_CASE("off by default") {
    list_app app;
    app.type("j");
    CHECK(app.selected == 0);
  }

  TEST_CASE("j k g G") {
    list_app app;
    enable_vim_keys(true);
    app.type("j");
    app.type("j");
    CHECK(app.selected == 2);
    app.type("k");
    CHECK(app.selected == 1);
    app.type("G");
    CHECK(app.selected == 3);
    app.type("g");
    CHECK(app.selected == 0);
  }
}

} // namespace xxx::testing
