// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <format>
#include <functional>
#include <utility>
#include <string>

#include "test_headless.h"

namespace xxx::testing {

namespace {

// two views with buttons; `request` is called inside view "b" before its buttons
struct focus_app : headless_app {
  int pressed_a = -1, pressed_b = -1;
  bool focused_b1 = false;
  std::function<void()> request_in_b = [] {};
  std::function<void()> request_after = [] {};

  focus_app() : headless_app(im_vec2(30, 10)) {
    step();
  }
  void step() {
    frame([this] {
      pressed_a = pressed_b = -1;
      view_begin("a");
      for (int i = 0; i < 2; ++i) {
        if (button(std::format("a{}", i))) {
          pressed_a = i;
        }
      }
      view_end();
      view_begin("b");
      std::exchange(request_in_b, [] {})();
      for (int i = 0; i < 3; ++i) {
        if (button(std::format("b{}", i))) {
          pressed_b = i;
        }
        if (i == 1) {
          focused_b1 = is_item_focused();
        }
      }
      std::exchange(request_after, [] {})();
      view_end();
    });
  }
  void enter() {
    backend().push_key(im_key_id::enter);
    step();
  }
};

} // namespace

TEST_SUITE("set_focus") {

  TEST_CASE("focus widget in inactive view: view is activated") {
    focus_app app;
    app.request_in_b = [] { set_focus("b2"); };
    app.step();
    CHECK(app.fg(0, 4) == test_inactive); // applied on next frame
    app.step();
    CHECK(app.fg(0, 4) == test_active);
    CHECK(app.fg(0, 0) == test_inactive);
    app.enter();
    CHECK(app.pressed_b == 2);
    CHECK(app.pressed_a == -1);
  }

  TEST_CASE("request after the widget was built takes one more frame") {
    focus_app app;
    app.request_after = [] { set_focus("b1"); };
    app.step(); // b1 already built: matched on next frame
    app.step(); // matched
    app.enter(); // applied at start of this frame
    CHECK(app.pressed_b == 1);
  }

  TEST_CASE("is_item_focused") {
    focus_app app;
    CHECK_FALSE(app.focused_b1);
    app.request_in_b = [] { set_focus("b1"); };
    app.step();
    app.step();
    CHECK(app.focused_b1);
  }

  TEST_CASE("id is resolved in current scope") {
    headless_app app(im_vec2(20, 4));
    int pressed = -1;
    bool request = true;
    auto const ui = [&] {
      pressed = -1;
      view_begin("v", 0);
      for (int i = 0; i < 3; ++i) {
        push_id(i);
        if (request && i == 2) {
          set_focus("x");
        }
        if (button("x")) {
          pressed = i;
        }
        pop_id();
      }
      view_end();
      request = false;
    };
    app.frame(ui);
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == 2);
  }

  TEST_CASE("request for missing widget expires") {
    headless_app app(im_vec2(20, 4));
    bool show = false;
    int pressed = -1;
    auto const ui = [&] {
      pressed = -1;
      view_begin("v", 0);
      if (button("first")) {
        pressed = 0;
      }
      if (show && button("late")) {
        pressed = 1;
      }
      view_end();
    };
    app.frame([&] {
      set_focus("late");
      ui();
    });
    app.frame(ui);
    app.frame(ui);
    show = true; // appears much later: must not steal focus
    app.frame(ui);
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == 0);
  }

  TEST_CASE("focused widget is scrolled into view") {
    headless_app app(im_vec2(16, 6));
    bool request = true;
    auto const ui = [&] {
      view_begin("v", im_view_flags_default, {}, 4); // 2 visible rows
      if (std::exchange(request, false)) {
        set_focus("b5");
      }
      for (int i = 0; i < 6; ++i) {
        button(std::format("b{}", i));
      }
      view_end();
    };
    app.frame(ui);
    app.frame(ui); // focus applied
    app.frame(ui); // scroll applied
    CHECK(app.line(2).find("[ b5 ]") != std::string::npos);
  }
}

TEST_SUITE("set_focus_next") {

  TEST_CASE("next widget gets focus") {
    headless_app app(im_vec2(20, 4));
    std::string a = "a", b = "b";
    bool request = true;
    auto const ui = [&] {
      view_begin("v", 0);
      text_input("first", a);
      if (std::exchange(request, false)) {
        set_focus_next();
      }
      text_input("second", b);
      view_end();
    };
    app.frame(ui);
    REQUIRE(app.reversed_cells().front().y == 0);
    app.frame(ui);
    CHECK(app.reversed_cells().front().y == 1);
    app.backend().push_text("!");
    app.frame(ui);
    CHECK(b == "b!");
    CHECK(a == "a");
  }

  TEST_CASE("next widget may be in another view") {
    headless_app app(im_vec2(20, 4));
    int pressed = -1;
    bool request = true;
    auto const ui = [&] {
      pressed = -1;
      view_begin("v", 0);
      if (button("one")) {
        pressed = 1;
      }
      if (std::exchange(request, false)) {
        set_focus_next();
      }
      view_end();
      view_begin("w", 0);
      if (button("two")) {
        pressed = 2;
      }
      view_end();
    };
    app.frame(ui);
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == 2);
  }

  TEST_CASE("flag does not survive the frame") {
    headless_app app(im_vec2(20, 4));
    int pressed = -1;
    bool request = true;
    auto const ui = [&] {
      pressed = -1;
      view_begin("v", 0);
      if (button("one")) {
        pressed = 1;
      }
      if (button("two")) {
        pressed = 2;
      }
      view_end();
      if (std::exchange(request, false)) {
        set_focus_next(); // nothing follows: first widget of next frame must not be targeted
      }
    };
    app.frame(ui);
    app.backend().push_key(im_key_id::tab); // focus "two"
    app.frame(ui);
    app.frame(ui);
    app.backend().push_key(im_key_id::enter);
    app.frame(ui);
    CHECK(pressed == 2);
  }
}

TEST_SUITE("set_focus with popup") {

  struct popup_app : headless_app {
    int pressed = -1;
    std::function<void()> in_popup = [] {};
    std::function<void()> in_view = [] {};

    popup_app() : headless_app(im_vec2(30, 10)) {
      frame([this] {
        open_popup("p");
        ui();
      });
      step();
    }
    void ui() {
      pressed = -1;
      view_begin("v");
      std::exchange(in_view, [] {})();
      if (button("bg")) {
        pressed = 0;
      }
      view_end();
      if (popup_begin("p", "p", 26)) {
        std::exchange(in_popup, [] {})();
        if (button("yes")) {
          pressed = 1;
        }
        same_line();
        if (button("no")) {
          pressed = 2;
          close_popup();
        }
        popup_end();
      }
    }
    void step() {
      frame([this] { ui(); });
    }
  };

  TEST_CASE("focus inside popup works") {
    popup_app app;
    app.in_popup = [] { set_focus("no"); };
    app.step();
    app.step();
    app.backend().push_key(im_key_id::enter);
    app.step();
    CHECK(app.pressed == 2);
  }

  TEST_CASE("focus outside open popup is ignored, base view stays usable after close") {
    popup_app app;
    app.in_view = [] { set_focus("bg"); };
    app.step();
    app.step();
    app.backend().push_key(im_key_id::enter);
    app.step();
    CHECK(app.pressed == 1); // still on popup's first button

    app.backend().push_key(im_key_id::esc);
    app.step();
    app.backend().push_key(im_key_id::enter);
    app.step();
    CHECK(app.pressed == 0);
  }
}

} // namespace xxx::testing
