// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <array>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "test_headless.h"

namespace xxx::testing {

using namespace std::string_view_literals;

TEST_SUITE("layout fill column") {

  TEST_CASE("fill takes the rest of the row") {
    headless_app app(im_vec2(30, 3));
    app.frame([] {
      layout_row_begin(2);
      layout_row_push(8);
      view_begin("a");
      view_end();
      layout_row_push(fill());
      view_begin("b");
      view_end();
      layout_row_end();
    });
    CHECK(app.line(0) == "╭─ a ──╮╭──────── b ─────────╮");
  }

  TEST_CASE("fill(n) leaves n columns for the next column") {
    headless_app app(im_vec2(30, 3));
    app.frame([] {
      layout_row_begin(2);
      layout_row_push(fill(10));
      view_begin("a");
      view_end();
      layout_row_push(fill());
      view_begin("b");
      view_end();
      layout_row_end();
    });
    CHECK(app.line(0) == "╭─────── a ────────╮╭── b ───╮");
  }
}

TEST_SUITE("set_next_item_width") {

  TEST_CASE("text_input fixed width") {
    headless_app app(im_vec2(30, 2));
    std::string text = "0123456789";
    app.frame([&] {
      view_begin("v", 0);
      set_next_item_width(6);
      text_input("t", text);
      same_line();
      label("|");
      view_end();
    });
    // 6 cells: prompt + "789" + cursor, then 1 cell spacing
    CHECK(app.line(0) == "> 789  |");
  }

  TEST_CASE("text_input fill width") {
    headless_app app(im_vec2(30, 2));
    std::string text = "x";
    app.frame([&] {
      view_begin("v", 0);
      label("name:");
      same_line();
      set_next_item_width(fill(2));
      text_input("t", text);
      same_line();
      label("<");
      view_end();
    });
    CHECK(app.line(0) == "name: > x                    <");
  }

  TEST_CASE("width applies to the next widget only") {
    headless_app app(im_vec2(30, 3));
    std::string a = "a", b = "b";
    app.frame([&] {
      view_begin("v", 0);
      set_next_item_width(4);
      text_input("a", a);
      same_line();
      label("|");
      text_input("b", b);
      same_line();
      label("|");
      view_end();
    });
    CHECK(app.line(0) == "> a  |");
    CHECK(app.line(1) == "> b              |"); // default 16
  }

  TEST_CASE("list width") {
    headless_app app(im_vec2(20, 2));
    auto const items = std::to_array({"one"sv});
    int selected = 0;
    app.frame([&] {
      view_begin("v", 0);
      set_next_item_width(5);
      list("l", items, selected);
      view_end();
    });
    CHECK(app.reversed_cells().size() == 5);
  }
}

TEST_SUITE("text_input extras") {

  struct input_app : headless_app {
    std::string text;
    int flags = 0;

    explicit input_app(std::string initial, int f = 0) : headless_app(im_vec2(20, 2)), text(std::move(initial)), flags(f) {
      step();
    }
    void step() {
      frame([this] {
        view_begin("v", 0);
        text_input("ph", text, flags);
        view_end();
      });
    }
    void key(im_key_id k) {
      backend().push_key(k);
      step();
    }
    auto cursor() const -> int {
      auto const cells = reversed_cells();
      REQUIRE(cells.size() == 1);
      return cells[0].x - 2;
    }
  };

  TEST_CASE("password is masked, value is kept") {
    input_app app("secret", im_input_flag_password);
    CHECK(app.line(0) == "> ******");
    app.backend().push_text("日");
    app.step();
    CHECK(app.text == "secret日");
    CHECK(app.line(0) == "> *******"); // wide char masked into one cell
    CHECK(app.cursor() == 7);
  }

  TEST_CASE("unfocused password is masked too") {
    headless_app app(im_vec2(20, 3));
    std::string a, b = "pw";
    app.frame([&] {
      view_begin("v", 0);
      text_input("a", a);
      text_input("b", b, im_input_flag_password);
      view_end();
    });
    CHECK(app.line(1) == "> **");
  }

  TEST_CASE("ctrl-a / ctrl-e") {
    input_app app("hello");
    app.key(im_key_id::ctrl_a);
    CHECK(app.cursor() == 0);
    app.key(im_key_id::ctrl_e);
    CHECK(app.cursor() == 5);
  }

  TEST_CASE("ctrl-u kills to start, ctrl-k to end") {
    input_app app("hello world");
    for (int i = 0; i < 5; ++i) {
      app.backend().push_key(im_key_id::arrow_left);
    }
    app.step(); // cursor before "world"
    app.key(im_key_id::ctrl_k);
    CHECK(app.text == "hello ");
    app.key(im_key_id::arrow_left);
    app.key(im_key_id::ctrl_u);
    CHECK(app.text == " ");
    CHECK(app.cursor() == 0);
    app.key(im_key_id::ctrl_u); // nothing before cursor
    CHECK(app.text == " ");
  }
}

TEST_SUITE("shift-tab") {

  struct three_buttons : headless_app {
    int pressed = -1;

    three_buttons() : headless_app(im_vec2(20, 4)) {
      step();
    }
    void step() {
      frame([this] {
        pressed = -1;
        view_begin("v", 0);
        for (int i = 0; i < 3; ++i) {
          if (button(std::format("b{}", i))) {
            pressed = i;
          }
        }
        view_end();
      });
    }
    auto focused() -> int {
      backend().push_key(im_key_id::enter);
      step();
      return pressed;
    }
  };

  TEST_CASE("moves focus backwards and wraps") {
    three_buttons app;
    REQUIRE(app.focused() == 0);
    app.backend().push_key(im_key_id::back_tab);
    app.step();
    CHECK(app.focused() == 2); // wrapped
    app.backend().push_key(im_key_id::back_tab);
    app.step();
    CHECK(app.focused() == 1);
  }

  TEST_CASE("tab and shift-tab are inverse") {
    three_buttons app;
    app.backend().push_key(im_key_id::tab);
    app.step();
    app.backend().push_key(im_key_id::tab);
    app.step();
    app.backend().push_key(im_key_id::back_tab);
    app.step();
    CHECK(app.focused() == 1);
  }
}

TEST_SUITE("push_id / pop_id") {

  TEST_CASE("same labels in different scopes are different widgets") {
    headless_app app(im_vec2(20, 4));
    std::array<bool, 3> pressed{};
    auto const ui = [&] {
      view_begin("v", 0);
      for (int i = 0; i < 3; ++i) {
        push_id(i);
        pressed[std::size_t(i)] = button("delete");
        pop_id();
      }
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::tab);
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == std::array{false, true, false});
  }

  TEST_CASE("string scopes") {
    headless_app app(im_vec2(20, 4));
    bool a = false, b = false;
    auto const ui = [&] {
      view_begin("v", 0);
      push_id("first");
      a = button("x");
      pop_id();
      push_id("second");
      b = button("x");
      pop_id();
      view_end();
    };
    app.frame(ui);
    app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(3, 1));
    app.frame(ui);
    CHECK_FALSE(a);
    CHECK(b);
  }
}

TEST_SUITE("tabs") {

  constexpr auto names = std::to_array({"logs"sv, "stats"sv, "help"sv});

  struct tabs_app : headless_app {
    int selected = 0;
    bool changed = false;

    explicit tabs_app(int initial = 0) : headless_app(im_vec2(30, 3)), selected(initial) {
      step();
    }
    void step() {
      frame([this] {
        view_begin("v", 0);
        changed = tabs("tabs", names, selected);
        label(std::format("page {}", selected));
        view_end();
      });
    }
    void key(im_key_id k, int times = 1) {
      for (int i = 0; i < times; ++i) {
        backend().push_key(k);
      }
      step();
    }
    auto highlighted() const -> std::string {
      auto result = std::string();
      for (auto const p : reversed_cells()) {
        result += backend_cell_char(p);
      }
      return result;
    }
    auto backend_cell_char(im_vec2 p) const -> char {
      return char(const_cast<tabs_app*>(this)->backend().cell(p.x, p.y).ch);
    }
  };

  TEST_CASE("rendering") {
    tabs_app app;
    CHECK(app.line(0) == " logs   stats   help");
    CHECK(app.highlighted() == " logs ");
  }

  TEST_CASE("arrows switch and wrap") {
    tabs_app app;
    app.key(im_key_id::arrow_right);
    CHECK(app.selected == 1);
    CHECK(app.changed);
    CHECK(app.highlighted() == " stats ");
    CHECK(app.line(1) == "page 1");
    app.step();
    CHECK_FALSE(app.changed);
    app.key(im_key_id::arrow_right, 2);
    CHECK(app.selected == 0);
    app.key(im_key_id::arrow_left);
    CHECK(app.selected == 2);
  }

  TEST_CASE("click selects tab under mouse") {
    tabs_app app;
    app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(16, 0)); // inside " help "
    app.step();
    CHECK(app.selected == 2);
    CHECK(app.changed);
    app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(6, 0)); // gap between tabs
    app.step();
    CHECK(app.selected == 2);
    CHECK_FALSE(app.changed);
  }

  TEST_CASE("out of range selection is clamped silently") {
    tabs_app app(9);
    CHECK(app.selected == 2);
    CHECK_FALSE(app.changed);
  }

  TEST_CASE("std::string items") {
    headless_app app(im_vec2(20, 2));
    auto const items = std::vector<std::string>{"日本", "b"};
    int selected = 1;
    app.frame([&] {
      view_begin("v", 0);
      tabs("t", items, selected);
      view_end();
    });
    CHECK(app.line(0) == " 日本   b");
  }
}

} // namespace xxx::testing
