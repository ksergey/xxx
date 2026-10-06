// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>

#include "test_headless.h"

namespace xxx::testing {

TEST_SUITE("lifecycle") {

  TEST_CASE("init / shutdown can be repeated") {
    for (int i = 0; i < 3; ++i) {
      headless_app app(im_vec2(10, 2));
      app.frame([] { label("hi"); });
      CHECK(app.screen() == "hi");
    }
  }

  TEST_CASE("shutdown without init is no-op") {
    shutdown();
    shutdown();
  }

  TEST_CASE("screen rect follows backend size") {
    headless_app app(im_vec2(12, 5));
    CHECK(get_screen_rect() == im_rect(0, 0, 11, 4));
    app.backend().resize(im_vec2(20, 3));
    CHECK(get_screen_rect() == im_rect(0, 0, 19, 2));
  }

  TEST_CASE("each frame is presented") {
    headless_app app(im_vec2(10, 2));
    app.frame([] {});
    app.frame([] {});
    CHECK(app.backend().frames() == 2);
  }
}

TEST_SUITE("view") {

  TEST_CASE("border and title") {
    headless_app app(im_vec2(20, 6));
    app.frame([] {
      view_begin("main");
      label("hello");
      label("world");
      view_end();
    });
    CHECK(app.screen() == dedent(R"(
      ╭────── main ──────╮
      │hello             │
      │world             │
      ╰──────────────────╯
    )"));
  }

  TEST_CASE("shortcut is shown in title") {
    headless_app app(im_vec2(20, 3));
    app.frame([] {
      view_begin("logs", im_key_id::ctrl_o);
      view_end();
    });
    CHECK(app.line(0) == "╭─── logs <c-o> ───╮");
  }

  TEST_CASE("title only") {
    headless_app app(im_vec2(12, 3));
    app.frame([] {
      view_begin("t", im_view_flag_title);
      label("x");
      view_end();
    });
    CHECK(app.screen() == dedent(R"(
           t
      x
    )"));
  }

  TEST_CASE("no decorations") {
    headless_app app(im_vec2(12, 3));
    app.frame([] {
      view_begin("t", 0);
      label("x");
      view_end();
    });
    CHECK(app.screen() == "x");
  }

  TEST_CASE("wide chars in title") {
    headless_app app(im_vec2(16, 3));
    app.frame([] {
      view_begin("日本");
      view_end();
    });
    CHECK(app.line(0) == "╭──── 日本 ────╮");
  }

  TEST_CASE("content is clipped by view border") {
    headless_app app(im_vec2(10, 3));
    app.frame([] {
      view_begin("v");
      label("0123456789abcdef");
      view_end();
    });
    CHECK(app.line(1) == "│01234567│");
  }

  TEST_CASE("first view is active") {
    headless_app app(im_vec2(20, 6));
    auto const ui = [] {
      view_begin("a");
      view_end();
      view_begin("b");
      view_end();
    };
    app.frame(ui);
    CHECK(app.fg(0, 0) == test_active);
    CHECK(app.fg(0, 2) == test_inactive);
  }

  TEST_CASE("shortcut activates view on next frame") {
    headless_app app(im_vec2(40, 3));
    auto const ui = [] {
      layout_row_begin(2);
      layout_row_push(0.5);
      view_begin("left", im_key_id::ctrl_a);
      view_end();
      layout_row_push(0.5);
      view_begin("right", im_key_id::ctrl_b);
      view_end();
      layout_row_end();
    };
    app.frame(ui);
    REQUIRE(app.fg(0, 0) == test_active);
    REQUIRE(app.fg(20, 0) == test_inactive);

    app.backend().push_key(im_key_id::ctrl_b);
    app.frame(ui);
    // switch is requested while building this frame, applied on the next one
    CHECK(app.fg(0, 0) == test_active);

    app.frame(ui);
    CHECK(app.fg(0, 0) == test_inactive);
    CHECK(app.fg(20, 0) == test_active);

    SUBCASE("shortcut of active view does nothing") {
      app.backend().push_key(im_key_id::ctrl_b);
      app.frame(ui);
      app.frame(ui);
      CHECK(app.fg(20, 0) == test_active);
    }
  }
}

TEST_SUITE("layout") {

  TEST_CASE("row with ratios") {
    headless_app app(im_vec2(20, 4));
    app.frame([] {
      layout_row_begin(2);
      layout_row_push(0.5);
      label("left");
      layout_row_push(0.5);
      label("right");
      layout_row_end();
      label("below");
    });
    CHECK(app.screen() == dedent(R"(
      left      right
      below
    )"));
  }

  TEST_CASE("row with fixed width") {
    headless_app app(im_vec2(20, 2));
    app.frame([] {
      layout_row_begin(2);
      layout_row_push(3);
      label("abcdef"); // columns do not clip: next column draws over
      layout_row_push(5);
      label("XY");
      layout_row_end();
    });
    CHECK(app.line(0).starts_with("abcXY"));
  }

  TEST_CASE("row height is the tallest column") {
    headless_app app(im_vec2(20, 5));
    app.frame([] {
      layout_row_begin(2);
      layout_row_push(0.5);
      label("a1");
      label("a2");
      label("a3");
      layout_row_push(0.5);
      label("b1");
      layout_row_end();
      label("after");
    });
    CHECK(app.screen() == dedent(R"(
      a1        b1
      a2
      a3
      after
    )"));
  }

  TEST_CASE("same_line") {
    headless_app app(im_vec2(20, 2));
    app.frame([] {
      label("one");
      same_line();
      label("two");
      label("three");
    });
    CHECK(app.screen() == "one two\nthree");
  }

  TEST_CASE("panel") {
    headless_app app(im_vec2(10, 4));
    app.frame([] {
      panel_begin();
      label("in");
      panel_end();
      label("out");
    });
    CHECK(app.screen() == dedent(R"(
      ╭────────╮
      │in      │
      ╰────────╯
      out
    )"));
  }
}

TEST_SUITE("label") {

  TEST_CASE("plain") {
    headless_app app(im_vec2(10, 1));
    app.frame([] { label("hello"); });
    CHECK(app.screen() == "hello");
  }

  TEST_CASE("wide chars take two cells") {
    headless_app app(im_vec2(12, 2));
    app.frame([] {
      label("日本語");
      same_line();
      label("|");
    });
    CHECK(app.line(0) == "日本語 |");
  }

  TEST_CASE("clipped at screen edge, wide char cut is replaced with space") {
    headless_app app(im_vec2(5, 1));
    app.frame([] { label("ab日本"); });
    CHECK(app.line(0) == "ab日");
    CHECK(app.backend().cell(4, 0).ch == ' ');
  }
}

TEST_SUITE("button") {

  auto const ok_cancel = [](bool& ok, bool& cancel) {
    return [&] {
      view_begin("v", 0);
      ok = button("ok");
      cancel = button("cancel");
      view_end();
    };
  };

  TEST_CASE("rendering") {
    headless_app app(im_vec2(20, 3));
    app.frame([] {
      button("ok");
      button("wide label here");
      button("ボタン");
    });
    CHECK(app.screen() == dedent(R"(
        [ ok ]
      [ wide label here ]
      [ ボタン ]
    )"));
  }

  TEST_CASE("first focusable widget gets focus") {
    headless_app app(im_vec2(20, 3));
    bool ok = false, cancel = false;
    app.frame(ok_cancel(ok, cancel));
    app.frame(ok_cancel(ok, cancel));
    CHECK(app.fg(4, 0) == test_active);   // "ok"
    CHECK(app.fg(2, 1) == test_inactive); // "cancel"
  }

  TEST_CASE("enter and space press focused button") {
    headless_app app(im_vec2(20, 3));
    bool ok = false, cancel = false;
    auto const ui = ok_cancel(ok, cancel);
    app.frame(ui);
    CHECK_FALSE(ok);

    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(ok);
    CHECK_FALSE(cancel);

    app.frame(ui);
    CHECK_FALSE(ok); // press lasts one frame

    app.backend().push_char(' ');
    app.frame(ui);
    CHECK(ok);
  }

  TEST_CASE("same label with different ids") {
    headless_app app(im_vec2(20, 3));
    bool a = false, b = false;
    auto const ui = [&] {
      view_begin("v", 0);
      a = button("go##1");
      b = button("go##2");
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::tab);
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK_FALSE(a);
    CHECK(b);
    CHECK(app.line(0) == "  [ go ]");
  }
}

TEST_SUITE("focus") {

  TEST_CASE("tab cycles through widgets and wraps") {
    headless_app app(im_vec2(20, 4));
    std::string text;
    bool b1 = false, b2 = false;
    auto const ui = [&] {
      view_begin("v", 0);
      text_input("name", text);
      b1 = button("one");
      b2 = button("two");
      view_end();
    };
    // reverse shows focus: the input's cursor or the focused button
    auto const focused = [&] {
      for (int row = 0; row < 3; ++row) {
        if (app.reversed_in_row(row)) {
          return row;
        }
      }
      return -1;
    };

    app.frame(ui);
    CHECK(focused() == 0);
    for (int expected : {1, 2, 0, 1}) {
      app.backend().push_key(im_key_id::tab);
      app.frame(ui);
      CHECK(focused() == expected);
    }
  }

  TEST_CASE("widgets in inactive view are never focused") {
    headless_app app(im_vec2(20, 6));
    bool a = false, b = false;
    auto const ui = [&] {
      view_begin("a");
      a = button("x##a");
      view_end();
      view_begin("b");
      b = button("x##b");
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(a);
    CHECK_FALSE(b);
    CHECK(app.fg(5, 4) == test_inactive);
  }
}

TEST_SUITE("text_input") {

  struct input_app : headless_app {
    std::string text;
    bool submitted = false;

    explicit input_app(std::string initial = {}) : headless_app(im_vec2(20, 2)), text(std::move(initial)) {
      step(); // focus + cursor at end
    }

    void step() {
      frame([this] {
        view_begin("v", 0);
        submitted = text_input("placeholder", text);
        view_end();
      });
    }
    void type(std::string_view s) {
      backend().push_text(s);
      step();
    }
    void key(im_key_id k, int times = 1) {
      for (int i = 0; i < times; ++i) {
        backend().push_key(k);
        step();
      }
    }
    // cursor column relative to text start (after "> ")
    auto cursor() const -> int {
      auto const cells = reversed_cells();
      REQUIRE(cells.size() == 1);
      return cells[0].x - 2;
    }
  };

  TEST_CASE("placeholder when empty") {
    input_app app;
    CHECK(app.line(0) == "> placeholder");
    CHECK(app.cursor() == 0); // first placeholder char shown as cursor
  }

  TEST_CASE("typing appends at cursor") {
    input_app app;
    app.type("abc");
    CHECK(app.text == "abc");
    CHECK(app.line(0) == "> abc");
    CHECK(app.cursor() == 3);
  }

  TEST_CASE("cursor starts at end of existing text") {
    input_app app("hello");
    CHECK(app.cursor() == 5);
  }

  TEST_CASE("cursor movement and insertion") {
    input_app app("ac");
    app.key(im_key_id::arrow_left);
    CHECK(app.cursor() == 1);
    app.type("b");
    CHECK(app.text == "abc");
    app.key(im_key_id::home);
    CHECK(app.cursor() == 0);
    app.key(im_key_id::arrow_left); // stays at 0
    CHECK(app.cursor() == 0);
    app.key(im_key_id::end);
    CHECK(app.cursor() == 3);
    app.key(im_key_id::arrow_right); // stays at end
    CHECK(app.cursor() == 3);
  }

  TEST_CASE("backspace and delete") {
    input_app app("abcd");
    app.key(im_key_id::backspace2);
    CHECK(app.text == "abc");
    app.key(im_key_id::home);
    app.key(im_key_id::del);
    CHECK(app.text == "bc");
    app.key(im_key_id::backspace2); // at start: nothing
    CHECK(app.text == "bc");
    app.key(im_key_id::end);
    app.key(im_key_id::del); // at end: nothing
    CHECK(app.text == "bc");
  }

  TEST_CASE("ctrl-w deletes previous word") {
    input_app app("foo bar baz");
    app.key(im_key_id::ctrl_w);
    CHECK(app.text == "foo bar ");
    app.key(im_key_id::ctrl_w);
    CHECK(app.text == "foo ");
    app.key(im_key_id::ctrl_w);
    CHECK(app.text == "");
  }

  TEST_CASE("ctrl-w with non-ascii") {
    input_app app("привет мир");
    app.key(im_key_id::ctrl_w);
    CHECK(app.text == "привет ");
  }

  TEST_CASE("enter submits") {
    input_app app("x");
    CHECK_FALSE(app.submitted);
    app.key(im_key_id::enter);
    CHECK(app.submitted);
    app.step();
    CHECK_FALSE(app.submitted);
  }

  TEST_CASE("long text scrolls to keep cursor visible") {
    input_app app;
    app.type("0123456789abcdefghij");
    REQUIRE(app.text == "0123456789abcdefghij");
    // field has 14 columns for text, last one is taken by cursor
    CHECK(app.cursor() == 13);
    CHECK(app.line(0) == "> 789abcdefghij");

    app.key(im_key_id::home);
    CHECK(app.cursor() == 0);
    CHECK(app.line(0) == "> 0123456789abcd");
  }

  TEST_CASE("wide chars: cursor moves by cells") {
    input_app app("日本");
    CHECK(app.cursor() == 4);
    app.key(im_key_id::arrow_left);
    CHECK(app.cursor() == 2);
    app.key(im_key_id::arrow_left);
    CHECK(app.cursor() == 0);
    CHECK(app.line(0) == "> 日本");
  }

  TEST_CASE("long paste in one frame is not truncated") {
    input_app app;
    auto const text = std::string(200, 'z') + "!";
    app.type(text);
    CHECK(app.text == text);
  }

  TEST_CASE("unfocused input shows text without cursor") {
    headless_app app(im_vec2(20, 3));
    std::string a = "first", b = "second";
    app.frame([&] {
      view_begin("v", 0);
      text_input("a", a);
      text_input("b", b);
      view_end();
    });
    CHECK(app.screen() == "> first\n> second");
    REQUIRE(app.reversed_cells().size() == 1);
    CHECK(app.reversed_cells()[0].y == 0);
  }
}

TEST_SUITE("spinner / progress / canvas") {

  TEST_CASE("spinner advances with time") {
    headless_app app(im_vec2(12, 1));
    float step = 0.0f;
    auto const ui = [&] { spinner("load", step); };
    app.frame(ui);
    CHECK(app.line(0) == "⣽ load");
    app.backend().advance_time(std::chrono::milliseconds(100));
    app.frame(ui);
    CHECK(app.line(0) == "⣻ load");
    app.backend().advance_time(std::chrono::milliseconds(600));
    app.frame(ui);
    CHECK(app.line(0) == "⣽ load"); // 7 glyphs: wrapped around
  }

  TEST_CASE("progress") {
    headless_app app(im_vec2(12, 3));
    app.frame([] {
      progress(0.0f);
      progress(50.0f);
      progress(250.0f); // clamped
    });
    CHECK(app.screen() == dedent(R"(

      ⣿⣿⣿⣿⣿
      ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
    )"));
  }

  TEST_CASE("invisible canvas does not leak clip rect") {
    headless_app app(im_vec2(10, 6));
    auto const ui = [] {
      view_begin("v", 0, {}, 2);
      label("top");
      // canvas below viewport: begin returns false, end must not be called
      label("pad");
      if (canvas_begin(im_vec2(4, 4))) {
        canvas_point(im_vec2(0, 0));
        canvas_end();
      }
      view_end();
      label("after");
    };
    app.frame(ui);
    CHECK(app.screen() == "top\npad\nafter");
  }

  TEST_CASE("canvas size rounds up to whole cells") {
    headless_app app(im_vec2(10, 3));
    app.frame([] {
      if (canvas_begin(im_vec2(5, 5))) { // 3 x 2 cells
        canvas_point(im_vec2(4, 4));     // last pixel: cell (2, 1)
        canvas_end();
      }
      label("after");
    });
    CHECK(app.screen() == "⠀⠀⠀\n⠀⠀⠁\nafter");
  }

  TEST_CASE("canvas points map to braille dots") {
    headless_app app(im_vec2(10, 2));
    app.frame([] {
      if (canvas_begin(im_vec2(4, 4))) {
        canvas_point(im_vec2(0, 0));
        canvas_point(im_vec2(3, 3));
        canvas_point(im_vec2(100, 100)); // outside: ignored
        canvas_end();
      }
      label("after");
    });
    CHECK(app.screen() == "⠁⢀\nafter");
  }
}

} // namespace xxx::testing
