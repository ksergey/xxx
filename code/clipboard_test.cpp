// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <array>
#include <string>
#include <string_view>

#include "base64.h"
#include "test_headless.h"

namespace xxx::testing {

using namespace std::string_view_literals;

TEST_SUITE("base64") {

  TEST_CASE("RFC 4648 test vectors") {
    CHECK(base64_encode("") == "");
    CHECK(base64_encode("f") == "Zg==");
    CHECK(base64_encode("fo") == "Zm8=");
    CHECK(base64_encode("foo") == "Zm9v");
    CHECK(base64_encode("foob") == "Zm9vYg==");
    CHECK(base64_encode("fooba") == "Zm9vYmE=");
    CHECK(base64_encode("foobar") == "Zm9vYmFy");
  }

  TEST_CASE("utf8 and high bytes") {
    CHECK(base64_encode("привет") == "0L/RgNC40LLQtdGC");
    CHECK(base64_encode("\xff\xfe") == "//4=");
  }
}

TEST_SUITE("osc52") {

  TEST_CASE("plain terminal") {
    CHECK(osc52_sequence("hi", false) == "\x1b]52;c;aGk=\x07");
  }

  TEST_CASE("tmux: plain form, then DCS passthrough with doubled ESC") {
    CHECK(osc52_sequence("hi", true) == "\x1b]52;c;aGk=\x07"
                                        "\x1bPtmux;\x1b\x1b]52;c;aGk=\x07\x1b\\");
  }
}

TEST_SUITE("clipboard") {

  TEST_CASE("set_clipboard") {
    headless_app app(im_vec2(10, 2));
    set_clipboard("copied 日本");
    CHECK(app.backend().clipboard() == "copied 日本");
  }

  struct input_app : headless_app {
    std::string text;
    std::string other;
    int flags = 0;

    explicit input_app(std::string initial, int f = 0) : headless_app(im_vec2(20, 3)), text(std::move(initial)), flags(f) {
      step();
    }
    void step() {
      frame([this] {
        view_begin("v", 0);
        text_input("a", text, flags);
        text_input("b", other);
        view_end();
      });
    }
    void key(im_key_id k, int times = 1) {
      for (int i = 0; i < times; ++i) {
        backend().push_key(k);
        step();
      }
    }
  };

  TEST_CASE("ctrl-c copies whole input") {
    input_app app("hello мир");
    app.key(im_key_id::ctrl_c);
    CHECK(app.backend().clipboard() == "hello мир");
    CHECK(app.text == "hello мир");
  }

  TEST_CASE("killed text goes to clipboard, ctrl-y yanks it") {
    input_app app("foo bar");
    app.key(im_key_id::ctrl_w);
    CHECK(app.text == "foo ");
    CHECK(app.backend().clipboard() == "bar");
    app.key(im_key_id::ctrl_a);
    app.key(im_key_id::ctrl_y);
    CHECK(app.text == "barfoo ");

    app.key(im_key_id::ctrl_k); // cursor after "bar"
    CHECK(app.text == "bar");
    CHECK(app.backend().clipboard() == "foo ");
    app.key(im_key_id::ctrl_u);
    CHECK(app.text == "");
    CHECK(app.backend().clipboard() == "bar");
  }

  TEST_CASE("kill buffer is shared between inputs") {
    input_app app("secret-free text");
    app.key(im_key_id::ctrl_a);
    app.key(im_key_id::ctrl_k);
    app.key(im_key_id::tab);
    app.key(im_key_id::ctrl_y);
    CHECK(app.other == "secret-free text");
  }

  TEST_CASE("ctrl-y with empty kill buffer does nothing") {
    input_app app("x");
    app.key(im_key_id::ctrl_y);
    CHECK(app.text == "x");
  }

  TEST_CASE("password never reaches clipboard or kill buffer") {
    input_app app("hunter2", im_input_flag_password);
    set_clipboard("before");
    app.key(im_key_id::ctrl_c);
    app.key(im_key_id::ctrl_w);
    app.key(im_key_id::ctrl_u);
    CHECK(app.text == "");
    CHECK(app.backend().clipboard() == "before");

    // nothing to yank into a plain field
    app.key(im_key_id::tab);
    app.key(im_key_id::ctrl_y);
    CHECK(app.other == "");
  }

  TEST_CASE("list: ctrl-c copies selected item") {
    headless_app app(im_vec2(20, 4));
    auto const items = std::to_array({"one"sv, "two"sv, "three"sv});
    int selected = 1;
    auto const ui = [&] {
      view_begin("v", 0);
      list("l", items, selected);
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::ctrl_c);
    app.frame(ui);
    CHECK(app.backend().clipboard() == "two");
  }

  TEST_CASE("table: ctrl-c copies row tab separated") {
    headless_app app(im_vec2(20, 4));
    auto const cols = std::to_array<im_table_column>({{"a", fill()}, {"b", 5}});
    auto const cells = std::to_array({"x"sv, "1"sv, "y"sv, "2"sv});
    int selected = 1;
    auto const ui = [&] {
      view_begin("v", 0);
      table("t", cols, cells, selected);
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::ctrl_c);
    app.frame(ui);
    CHECK(app.backend().clipboard() == "y\t2");
  }

  TEST_CASE("unfocused list does not copy") {
    headless_app app(im_vec2(20, 4));
    auto const items = std::to_array({"one"sv});
    int selected = 0;
    auto const ui = [&] {
      view_begin("v", 0);
      button("b");
      list("l", items, selected);
      view_end();
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::ctrl_c);
    app.frame(ui);
    CHECK(app.backend().clipboard() == "");
  }
}

} // namespace xxx::testing
