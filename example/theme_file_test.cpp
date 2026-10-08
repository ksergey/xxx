// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <cstdio>
#include <fstream>
#include <string>

#include "test_headless.h"
#include "theme_file.h"

using theme_file::load_theme;
using theme_file::load_theme_file;

namespace xxx::testing {

TEST_SUITE("theme file") {

    TEST_CASE("colors, attributes and partial overrides") {
        headless_app app(im_vec2(10, 1));
        use_theme(im_theme_preset::terminal);
        auto const error = load_theme(R"({
      "accent": { "fg": "bright_magenta", "attrs": ["bold", "underline"] },
      "focus":  { "fg": "black", "bg": "#dcf763" },
      "text":   { "bg": 236 }
    })");
        REQUIRE(error == "");
        CHECK(*get_style(im_role::accent).fg == ansi::bright_magenta);
        CHECK(get_style(im_role::accent).attrs == (im_attr_bold | im_attr_underline));
        CHECK(*get_style(im_role::focus).bg == im_color(0xdcf763u));
        CHECK(get_style(im_role::focus).attrs == im_attr_reverse); // not given: kept
        CHECK(*get_style(im_role::text).bg == im_color::indexed(236));
        CHECK(*get_style(im_role::error).bg == im_color::indexed(236)); // inherited from text
    }

    TEST_CASE("base theme, default and null") {
        headless_app app(im_vec2(10, 1));
        REQUIRE(load_theme(R"({"base": "classic", "accent": {"fg": "default"}, "muted": {"fg": null}})") == "");
        CHECK(get_style(im_role::accent).fg->is_default());
        CHECK(*get_style(im_role::border_focused).fg == im_color(0xdcf763u)); // from classic
        CHECK(*get_style(im_role::muted).fg == *get_style(im_role::text).fg); // null: inherit from text
    }

    TEST_CASE("errors name the place and leave the theme untouched") {
        headless_app app(im_vec2(10, 1));
        use_theme(im_theme_preset::terminal);
        auto const check = [](std::string_view json, std::string_view expected) {
            auto const message = load_theme(json);
            CAPTURE(message);
            CHECK(message.find(expected) != std::string::npos);
            CHECK(*get_style(im_role::accent).fg == ansi::cyan); // unchanged
        };
        check(R"({"accent": {"fg": "red"}, "acent": {"fg": "red"}})", "unknown role \"acent\"");
        check(R"({"accent": {"fg": "reddish"}})", "accent.fg: unknown color \"reddish\"");
        check(R"({"accent": {"fg": "#12345"}})", "accent.fg: unknown color");
        check(R"({"accent": {"fg": 300}})", "accent.fg: palette index 300 is out of 0..255");
        check(R"({"accent": {"color": "red"}})", "accent.color: unknown field");
        check(R"({"accent": {"attrs": ["shiny"]}})", "accent.attrs: unknown attribute \"shiny\"");
        check(R"({"accent": "red"})", "accent: must be an object");
        check(R"({"base": "neon"})", "base: unknown theme");
        check(R"({"accent": {"fg": "red"},})", "invalid JSON");
        check(R"([1, 2])", "theme must be a JSON object");
    }

    TEST_CASE("from a file") {
        headless_app app(im_vec2(10, 1));
        auto const path = std::string("/tmp/xxx_theme_test.json");
        std::ofstream(path) << R"({"warning": {"fg": "bright_yellow"}})";
        CHECK(load_theme_file(path) == "");
        CHECK(*get_style(im_role::warning).fg == ansi::bright_yellow);
        std::remove(path.c_str());
        CHECK(load_theme_file(path).find("cannot read") != std::string::npos);
    }

    TEST_CASE("file errors include the path") {
        headless_app app(im_vec2(10, 1));
        auto const path = std::string("/tmp/xxx_theme_bad.json");
        std::ofstream(path) << R"({"oops": {}})";
        auto const message = load_theme_file(path);
        CHECK(message.starts_with(path + ": unknown role"));
        std::remove(path.c_str());
    }
}

} // namespace xxx::testing
