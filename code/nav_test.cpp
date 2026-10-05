// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <array>
#include <format>
#include <string>
#include <string_view>

#include "test_headless.h"

namespace xxx::testing {

using namespace std::string_view_literals;

namespace {

constexpr auto up = im_key_id::arrow_up;
constexpr auto down = im_key_id::arrow_down;
constexpr auto left = im_key_id::arrow_left;
constexpr auto right = im_key_id::arrow_right;

// 2x3 grid of buttons like life's controls:  a b / c d / e f
struct grid_app : headless_app {
  std::string pressed;

  grid_app() : headless_app(im_vec2(30, 5)) {
    step();
  }
  void step() {
    frame([this] {
      pressed.clear();
      view_begin("v", 0);
      for (auto const row : {"ab"sv, "cd"sv, "ef"sv}) {
        if (button(row.substr(0, 1))) {
          pressed = row.substr(0, 1);
        }
        same_line();
        if (button(row.substr(1, 1))) {
          pressed = row.substr(1, 1);
        }
      }
      view_end();
    });
  }
  // move with arrows, then press enter: which button has focus
  auto focus_after(std::initializer_list<im_key_id> keys) -> std::string {
    for (auto const k : keys) {
      backend().push_key(k);
      step();
    }
    backend().push_key(im_key_id::enter);
    step();
    return pressed;
  }
};

} // namespace

TEST_SUITE("arrow navigation") {

  TEST_CASE("grid of buttons") {
    grid_app app;
    CHECK(app.focus_after({}) == "a");
    CHECK(app.focus_after({right}) == "b");
    CHECK(app.focus_after({down}) == "d");      // straight down, not diagonal
    CHECK(app.focus_after({down}) == "f");
    CHECK(app.focus_after({left}) == "e");
    CHECK(app.focus_after({up, up}) == "a");
  }

  TEST_CASE("no wrap at edges") {
    grid_app app;
    CHECK(app.focus_after({left, up}) == "a");
    CHECK(app.focus_after({right, right, right}) == "b");
  }

  TEST_CASE("several presses in one frame") {
    grid_app app;
    app.backend().push_key(down);
    app.backend().push_key(down);
    app.step();
    CHECK(app.focus_after({}) == "e");
  }

  TEST_CASE("moves between views and activates the target view") {
    headless_app app(im_vec2(40, 4));
    int pressed = -1;
    auto const ui = [&] {
      pressed = -1;
      layout_row_begin(2);
      layout_row_push(ratio(0.5f));
      view_begin("left");
      if (button("l")) {
        pressed = 0;
      }
      view_end();
      layout_row_push(fill());
      view_begin("right");
      if (button("r")) {
        pressed = 1;
      }
      view_end();
      layout_row_end();
    };
    app.frame(ui);
    REQUIRE(app.fg(20, 0) == test_inactive);
    app.backend().push_key(right);
    app.frame(ui);
    CHECK(app.fg(20, 0) == test_active);
    CHECK(app.fg(0, 0) == test_inactive);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == 1);
  }

  TEST_CASE("list keeps up/down, left/right leave it") {
    headless_app app(im_vec2(40, 6));
    auto const items = std::to_array({"one"sv, "two"sv, "three"sv});
    int selected = 0;
    bool ok = false;
    auto const ui = [&] {
      layout_row_begin(2);
      layout_row_push(ratio(0.5f));
      view_begin("list");
      list("l", items, selected);
      view_end();
      layout_row_push(fill());
      view_begin("buttons");
      ok = button("ok");
      view_end();
      layout_row_end();
    };
    app.frame(ui);
    app.backend().push_key(down);
    app.frame(ui);
    CHECK(selected == 1); // the list used the arrow
    app.backend().push_key(right);
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(ok);
    CHECK(selected == 1);
  }

  TEST_CASE("arriving arrow is used up: tabs don't switch on arrival") {
    headless_app app(im_vec2(30, 4));
    auto const names = std::to_array({"a"sv, "b"sv});
    int tab = 0;
    auto const ui = [&] {
      view_begin("v", 0);
      button("go");
      same_line();
      tabs("t", names, tab);
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(right); // button -> tabs
    app.frame(ui);
    CHECK(tab == 0);
    app.backend().push_key(right); // now the tabs use it
    app.frame(ui);
    CHECK(tab == 1);
  }

  TEST_CASE("text input keeps left/right, up/down leave it") {
    headless_app app(im_vec2(30, 4));
    std::string a = "abc", b = "xyz";
    auto const ui = [&] {
      view_begin("v", 0);
      text_input("a", a);
      text_input("b", b);
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(left);
    app.frame(ui);
    app.backend().push_key(down);
    app.frame(ui);
    app.backend().push_text("!");
    app.frame(ui);
    CHECK(a == "abc");
    CHECK(b == "xyz!");
  }

  TEST_CASE("hidden widgets of a scrolled view are reachable, the view follows") {
    headless_app app(im_vec2(16, 6));
    int pressed = -1;
    auto const ui = [&] {
      pressed = -1;
      view_begin("v", im_view_flags_default, {}, 4); // 2 visible rows
      for (int i = 0; i < 5; ++i) {
        if (button(std::format("b{}", i))) {
          pressed = i;
        }
      }
      view_end();
    };
    app.frame(ui);
    for (int i = 0; i < 3; ++i) {
      app.backend().push_key(down);
      app.frame(ui);
    }
    app.frame(ui); // scroll is applied next frame
    CHECK(app.screen().find("[ b3 ]") != std::string::npos);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == 3);
  }

  TEST_CASE("text only view that scrolls is a stop, arrows scroll it by line") {
    headless_app app(im_vec2(40, 6));
    bool ok = false;
    auto const ui = [&] {
      layout_row_begin(2);
      layout_row_push(ratio(0.5f));
      view_begin("menu");
      ok = button("ok");
      view_end();
      layout_row_push(fill());
      view_begin("log", im_view_flags_default, {}, 4); // 2 visible rows, 6 lines
      for (int i = 0; i < 6; ++i) {
        label(std::format("line {}", i));
      }
      view_end();
      layout_row_end();
    };
    app.frame(ui);
    app.frame(ui); // log knows it overflows
    app.backend().push_key(right);
    app.frame(ui);
    CHECK(app.fg(20, 0) == test_active);
    app.backend().push_key(down);
    app.frame(ui);
    CHECK(app.line(1).find("line 1") != std::string::npos); // scrolled by one line
    app.backend().push_key(left); // back to the button
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(ok);
  }

  TEST_CASE("text that fits is not a stop") {
    headless_app app(im_vec2(40, 6));
    auto const ui = [&] {
      layout_row_begin(2);
      layout_row_push(ratio(0.5f));
      view_begin("menu");
      button("ok");
      view_end();
      layout_row_push(fill());
      view_begin("info", im_view_flags_default, {}, 4);
      label("short");
      view_end();
      layout_row_end();
    };
    app.frame(ui);
    app.frame(ui);
    app.backend().push_key(right);
    app.frame(ui);
    CHECK(app.fg(0, 0) == test_active); // stayed: nothing to do over there
  }

  TEST_CASE("popup keeps navigation inside") {
    headless_app app(im_vec2(30, 8));
    std::string pressed;
    auto const ui = [&] {
      pressed.clear();
      view_begin("v");
      if (button("bg")) {
        pressed = "bg";
      }
      view_end();
      if (popup_begin("p", "p", 26)) {
        if (button("yes")) {
          pressed = "yes";
        }
        same_line();
        if (button("no")) {
          pressed = "no";
        }
        popup_end();
      }
    };
    app.frame([&] {
      open_popup("p");
      ui();
    });
    app.frame(ui);
    for (auto const k : {right, up, up, left, right}) {
      app.backend().push_key(k);
      app.frame(ui);
    }
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == "no");
  }
}

} // namespace xxx::testing

namespace xxx::testing {

TEST_SUITE("arrow navigation: list boundaries") {

  struct sandwich_app : headless_app {
    int selected = 0;
    std::string pressed;

    explicit sandwich_app(int initial) : headless_app(im_vec2(20, 8)), selected(initial) {
      step();
    }
    void step() {
      static constexpr auto items = std::to_array({"one"sv, "two"sv, "three"sv});
      frame([this] {
        pressed.clear();
        view_begin("v", 0);
        if (button("above")) {
          pressed = "above";
        }
        list("l", items, selected);
        if (button("below")) {
          pressed = "below";
        }
        view_end();
      });
    }
    void key(im_key_id k) {
      backend().push_key(k);
      step();
    }
  };

  TEST_CASE("up inside the list moves the selection") {
    sandwich_app app(2);
    app.key(im_key_id::tab); // focus the list
    app.key(im_key_id::arrow_up);
    CHECK(app.selected == 1);
  }

  TEST_CASE("up on the first row leaves the list") {
    sandwich_app app(0);
    app.key(im_key_id::tab);
    app.key(im_key_id::arrow_up);
    app.key(im_key_id::enter);
    CHECK(app.pressed == "above");
    CHECK(app.selected == 0);
  }

  TEST_CASE("down on the last row leaves the list") {
    sandwich_app app(1);
    app.key(im_key_id::tab);
    app.key(im_key_id::arrow_down); // to the last row
    CHECK(app.selected == 2);
    app.key(im_key_id::arrow_down); // leaves
    app.key(im_key_id::enter);
    CHECK(app.pressed == "below");
  }
}

} // namespace xxx::testing
