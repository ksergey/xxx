// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <string_view>

#include "ansi_input.h"
#include "test_headless.h"

namespace xxx::testing {

namespace {

auto parse(std::string_view bytes) -> im_input {
  auto p = ansi_input_parser();
  auto in = im_input();
  in.reset();
  for (auto const ch : bytes) {
    p.feed(std::string_view(&ch, 1), in); // byte by byte: modifiers must survive splitting
  }
  p.flush(in);
  return in;
}

} // namespace

TEST_SUITE("modifiers: parser") {

  TEST_CASE("ctrl / shift / alt with arrows") {
    auto const in = parse("\x1b[1;5C\x1b[1;2A\x1b[1;3D\x1b[1;6B");
    CHECK(in.is_key_pressed(im_key_id::arrow_right, im_mod_ctrl));
    CHECK(in.is_key_pressed(im_key_id::arrow_up, im_mod_shift));
    CHECK(in.is_key_pressed(im_key_id::arrow_left, im_mod_alt));
    CHECK(in.is_key_pressed(im_key_id::arrow_down, im_mod_ctrl | im_mod_shift));
    CHECK_FALSE(in.is_key_pressed(im_key_id::arrow_right, 0));
    CHECK(in.is_key_pressed(im_key_id::arrow_right)); // any modifiers
  }

  TEST_CASE("modifiers on tilde keys and function keys") {
    auto const in = parse("\x1b[3;5~\x1b[15;2~\x1b[1;5P");
    CHECK(in.is_key_pressed(im_key_id::del, im_mod_ctrl));
    CHECK(in.is_key_pressed(im_key_id::f5, im_mod_shift));
    CHECK(in.is_key_pressed(im_key_id::f1, im_mod_ctrl));
  }

  TEST_CASE("no modifiers") {
    auto const in = parse("\x1b[C\x1b[1;1C");
    CHECK(in.is_key_pressed(im_key_id::arrow_right, 0));
  }

  TEST_CASE("alt + char is not text") {
    auto const in = parse("\x1bx\x1bX y");
    CHECK(in.is_alt_pressed('x'));
    CHECK(in.is_alt_pressed('X'));
    CHECK_FALSE(in.is_alt_pressed('y'));
    auto const events = in.get_input_events();
    REQUIRE(events.size() == 5); // alt+x, alt+X, ' ', space key, 'y'
    CHECK(events[0].mods == im_mod_alt);
    CHECK(events[4].mods == 0);
  }

  TEST_CASE("alt + backspace and alt + control key") {
    auto const in = parse("\x1b\x7f\x1b\x01");
    CHECK(in.is_key_pressed(im_key_id::backspace2, im_mod_alt));
    CHECK(in.is_key_pressed(im_key_id::ctrl_a, im_mod_alt));
  }

  TEST_CASE("alt prefix applies to one item only") {
    auto const in = parse("\x1b" "ab"); // split literal: "\x1bab" would be one hex escape
    auto const events = in.get_input_events();
    REQUIRE(events.size() == 2);
    CHECK(events[0].mods == im_mod_alt);
    CHECK(events[1].mods == 0);
  }

  TEST_CASE("lone esc is still the Esc key") {
    auto const in = parse("\x1b");
    CHECK(in.is_key_pressed(im_key_id::esc, 0));
  }
}

TEST_SUITE("modifiers: widgets") {

  struct input_app : headless_app {
    std::string text;

    explicit input_app(std::string initial) : headless_app(im_vec2(40, 2)), text(std::move(initial)) {
      step();
    }
    void step() {
      frame([this] {
        view_begin("v", 0);
        set_next_item_width(fill());
        text_input("t", text);
        view_end();
      });
    }
    void key(im_key_id k, int mods = 0) {
      backend().push_key(k, mods);
      step();
    }
    void alt(char ch) {
      backend().push_alt(std::uint32_t(ch));
      step();
    }
    auto cursor() const -> int {
      return reversed_cells().front().x - 2;
    }
  };

  TEST_CASE("alt + char doesn't type, public API sees it") {
    input_app app("abc");
    bool seen = false;
    app.backend().push_alt('s');
    app.frame([&] {
      seen = is_alt_pressed('s');
      view_begin("v", 0);
      text_input("t", app.text);
      view_end();
    });
    CHECK(seen);
    CHECK(app.text == "abc");
  }

  TEST_CASE("ctrl + left / right move by words") {
    input_app app("one two  three");
    app.key(im_key_id::arrow_left, im_mod_ctrl);
    CHECK(app.cursor() == 9); // start of "three"
    app.key(im_key_id::arrow_left, im_mod_ctrl);
    CHECK(app.cursor() == 4); // start of "two"
    app.key(im_key_id::arrow_right, im_mod_ctrl);
    CHECK(app.cursor() == 7); // end of "two"
    app.key(im_key_id::arrow_left); // plain arrow: one char
    CHECK(app.cursor() == 6);
  }

  TEST_CASE("alt+b / alt+f, readline style") {
    input_app app("hello wide world");
    app.alt('b');
    CHECK(app.cursor() == 11);
    app.alt('b');
    app.alt('f');
    CHECK(app.cursor() == 10);
  }

  TEST_CASE("alt + backspace deletes the word before the cursor") {
    input_app app("foo bar baz");
    app.key(im_key_id::backspace2, im_mod_alt);
    CHECK(app.text == "foo bar ");
  }

  TEST_CASE("ctrl + arrow still moves focus between widgets") {
    headless_app app(im_vec2(30, 3));
    int pressed = -1;
    auto const ui = [&] {
      pressed = -1;
      view_begin("v", 0);
      if (button("a")) {
        pressed = 0;
      }
      same_line();
      if (button("b")) {
        pressed = 1;
      }
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::arrow_right, im_mod_ctrl);
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == 1);
  }

  TEST_CASE("is_char_pressed: typed letters, not alt shortcuts") {
    headless_app app(im_vec2(10, 1));
    auto typed = false, alt = false;
    app.backend().push_text("r");
    app.frame([&] { typed = is_char_pressed('r'); });
    app.backend().push_alt('r');
    app.frame([&] { alt = is_char_pressed('r'); });
    CHECK(typed);
    CHECK_FALSE(alt);
  }

  TEST_CASE("alt + ? doesn't open help") {
    headless_app app(im_vec2(30, 3));
    auto const ui = [] {
      view_begin("v", 0);
      button("ok");
      view_end();
    };
    app.frame(ui);
    app.backend().push_alt('?');
    app.frame(ui);
    CHECK_FALSE(is_popup_open());
  }
}

} // namespace xxx::testing
