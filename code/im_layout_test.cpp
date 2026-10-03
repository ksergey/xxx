// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "im_layout.h"
#include "test_utils.h"

namespace xxx::testing {

namespace {

auto make_layout(im_rect const& screen = im_rect(0, 0, 79, 23)) -> im_layout {
  im_layout l;
  l.reset(screen);
  return l;
}

} // namespace

TEST_SUITE("im_layout") {

  TEST_CASE("reset creates root container") {
    auto l = make_layout(im_rect(2, 3, 50, 20));
    REQUIRE(l.layout_state_stack.size() == 1);
    CHECK(l.layout_state_stack.back().type == im_layout_type::container);
    CHECK(l.layout_state_stack.back().rect == im_rect(2, 3, 50, 20));
    CHECK(l.cursor == im_vec2(2, 3));
  }

  TEST_CASE("widgets stack vertically") {
    auto l = make_layout();
    CHECK(l.add_widget_item(im_vec2(5, 1)) == im_rect(0, 0, 4, 0));
    CHECK(l.add_widget_item(im_vec2(3, 2)) == im_rect(0, 1, 2, 2));
    CHECK(l.add_widget_item(im_vec2(7, 1)) == im_rect(0, 3, 6, 3));
  }

  TEST_CASE("same_line places widget to the right") {
    auto l = make_layout();
    CHECK(l.add_widget_item(im_vec2(5, 1)) == im_rect(0, 0, 4, 0));
    l.same_line = true;
    // spacing.x == 1
    CHECK(l.add_widget_item(im_vec2(3, 1)) == im_rect(6, 0, 8, 0));
    CHECK(l.widget_item.same_line);
    // one-shot flag
    CHECK(l.add_widget_item(im_vec2(2, 1)) == im_rect(0, 1, 1, 1));
    CHECK_FALSE(l.widget_item.same_line);
  }

  TEST_CASE("zero height widget is ignored") {
    auto l = make_layout();
    CHECK(l.add_widget_item(im_vec2(5, 0)).empty());
    CHECK(l.add_widget_item(im_vec2(5, 1)) == im_rect(0, 0, 4, 0));
  }

  TEST_CASE("reserve lines spans full width") {
    auto l = make_layout(im_rect(0, 0, 39, 9));
    CHECK(l.reserve_layout_lines(2) == im_rect(0, 0, 39, 1));
    CHECK(l.cursor.y == 2);
    CHECK(l.add_widget_item(im_vec2(4, 1)) == im_rect(0, 2, 3, 2));
  }

  TEST_CASE("widgets respect container origin") {
    auto l = make_layout(im_rect(10, 5, 40, 20));
    CHECK(l.add_widget_item(im_vec2(3, 1)) == im_rect(10, 5, 12, 5));
    CHECK(l.add_widget_item(im_vec2(3, 1)) == im_rect(10, 6, 12, 6));
  }
}

} // namespace xxx::testing
