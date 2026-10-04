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

namespace {

constexpr auto columns = std::to_array<im_table_column>({
    {"name", fill()},
    {"cpu", 5, im_align::right},
    {"st", 4, im_align::center},
});

// clang-format off
constexpr auto cells = std::to_array({
    "init"sv,       "0.1"sv,  "S"sv,
    "postgres"sv,   "12.5"sv, "R"sv,
    "nginx"sv,      "3.0"sv,  "S"sv,
    "very-long-process-name"sv, "100.0"sv, "Z"sv,
    "sshd"sv,       "0.0"sv,  "S"sv,
});
// clang-format on

struct table_app : headless_app {
  int selected = 0;
  int height = 3;
  bool activated = false;

  table_app() : headless_app(im_vec2(22, 6)) {
    step();
  }
  void step() {
    frame([this] {
      view_begin("v", 0);
      activated = table("procs", columns, cells, selected, height);
      label("--");
      view_end();
    });
  }
  void key(im_key_id k, int times = 1) {
    for (int i = 0; i < times; ++i) {
      backend().push_key(k);
    }
    step();
  }
  auto highlighted_row() const -> int {
    auto const cells = reversed_cells();
    return cells.empty() ? -1 : cells.front().y;
  }
};

} // namespace

TEST_SUITE("table") {

  TEST_CASE("rendering: header, alignment, clipping by column") {
    table_app app;
    // 22 cells: name 11, gap, cpu 5, gap, st 4
    CHECK(app.screen() == dedent(R"(
      name          cpu  st
      init          0.1  S
      postgres     12.5  R
      nginx         3.0  S
      --
    )"));
  }

  TEST_CASE("header is underlined") {
    table_app app;
    CHECK((app.backend().cell(0, 0).style.attrs & im_attr_underline) != 0);
    CHECK((app.backend().cell(0, 1).style.attrs & im_attr_underline) == 0);
  }

  TEST_CASE("long cell is cut by its own column") {
    table_app app;
    app.key(im_key_id::arrow_down, 3);
    CHECK(app.line(3) == "very-long-p 100.0  Z");
  }

  TEST_CASE("selection, scrolling and activation like list") {
    table_app app;
    CHECK(app.highlighted_row() == 1);
    app.key(im_key_id::end);
    CHECK(app.selected == 4);
    CHECK(app.line(0) == "name          cpu  st"); // header stays
    CHECK(app.line(1) == "nginx         3.0  S");
    CHECK(app.line(3) == "sshd          0.0  S");
    CHECK(app.highlighted_row() == 3);
    app.key(im_key_id::enter);
    CHECK(app.activated);
  }

  TEST_CASE("click on row selects, click on header does nothing") {
    table_app app;
    app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(3, 2));
    app.step();
    CHECK(app.selected == 1);
    CHECK(app.activated);
    app.backend().push_mouse_button(im_mouse_button_id::left, im_vec2(3, 0));
    app.step();
    CHECK(app.selected == 1);
    CHECK_FALSE(app.activated);
  }

  TEST_CASE("height 0 shows all rows") {
    table_app app;
    app.height = 0;
    app.step();
    CHECK(app.line(5) == "sshd          0.0  S");
  }

  TEST_CASE("ratio and fixed widths") {
    headless_app app(im_vec2(21, 3));
    auto const cols = std::to_array<im_table_column>({{"a", 0.5f}, {"b", 3}, {"c", fill()}});
    auto const data = std::to_array({"aaaaaaaaaaaaaaaa"sv, "bbbbb"sv, "ccccc"sv});
    int selected = -1;
    app.frame([&] {
      view_begin("v", 0);
      table("t", cols, data, selected);
      view_end();
    });
    // available 19: a = ceil(0.5 * 19) = 10, b = 3, c = rest 6
    CHECK(app.line(1) == "aaaaaaaaaa bbb ccccc");
  }

  TEST_CASE("std::string cells and wide chars") {
    headless_app app(im_vec2(12, 3));
    auto const cols = std::to_array<im_table_column>({{"k", 4}, {"v", fill(), im_align::right}});
    auto const data = std::vector<std::string>{"日本", "1"};
    int selected = 0;
    app.frame([&] {
      view_begin("v", 0);
      table("t", cols, data, selected);
      view_end();
    });
    CHECK(app.line(1) == "日本       1");
  }

  TEST_CASE("empty table shows header only") {
    headless_app app(im_vec2(12, 3));
    int selected = 2;
    app.frame([&] {
      view_begin("v", 0);
      table("t", columns, std::span<std::string const>(), selected, 1);
      label("x");
      view_end();
    });
    CHECK(selected == -1);
    // 12 cells: fill column gets what is left after cpu (5), st (4) and 2 gaps: 1 cell
    CHECK(app.screen() == "n   cpu  st\n\nx");
  }

  TEST_CASE("width of the table follows set_next_item_width") {
    headless_app app(im_vec2(30, 3));
    auto const cols = std::to_array<im_table_column>({{"a", fill()}, {"b", 2}});
    auto const data = std::to_array({"x"sv, "y"sv});
    int selected = 0;
    app.frame([&] {
      view_begin("v", 0);
      set_next_item_width(8);
      table("t", cols, data, selected);
      view_end();
    });
    CHECK(app.line(1) == "x     y");
    CHECK(app.reversed_cells().size() == 8);
  }
}

} // namespace xxx::testing
