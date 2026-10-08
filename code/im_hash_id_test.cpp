// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <new>
#include <string_view>

#include <doctest/doctest.h>

#include "im_hash_id.h"

namespace xxx::testing {

using namespace std::string_view_literals;

TEST_SUITE("im_hash_id") {

    TEST_CASE("split_str_key") {
        SUBCASE("without marker") {
            auto [text, key] = im_hash_id::split_str_key("world");
            CHECK(text == "world"sv);
            CHECK(key == "world"sv);
        }
        SUBCASE("with marker") {
            auto [text, key] = im_hash_id::split_str_key("hello##1234");
            CHECK(text == "hello"sv);
            CHECK(key == "##1234"sv);
        }
        SUBCASE("uses last marker") {
            auto [text, key] = im_hash_id::split_str_key("a##b##c");
            CHECK(text == "a##b"sv);
            CHECK(key == "##c"sv);
        }
        SUBCASE("hidden label") {
            auto [text, key] = im_hash_id::split_str_key("##only_key");
            CHECK(text == ""sv);
            CHECK(key == "##only_key"sv);
        }
        SUBCASE("constexpr") {
            constexpr auto r = im_hash_id::split_str_key("x##y");
            static_assert(std::get<0>(r) == "x"sv);
            static_assert(std::get<1>(r) == "##y"sv);
        }
    }

    // "foo##bar" must not collide with plain "bar"
    TEST_CASE("marker key does not collide with plain label") {
        im_hash_id h;
        h.reset();
        auto [_, key] = im_hash_id::split_str_key("foo##bar");
        CHECK(h.make(key) != h.make("bar"sv));
    }

    TEST_CASE("make is stable") {
        im_hash_id h;
        h.reset();
        CHECK(h.make("ok"sv) == h.make("ok"sv));
        CHECK(h.make("ok"sv) != h.make("cancel"sv));
    }

    TEST_CASE("reset seed changes ids") {
        im_hash_id a, b;
        a.reset(0);
        b.reset(1);
        CHECK(a.make("x"sv) != b.make("x"sv));
    }

    TEST_CASE("scope changes ids") {
        im_hash_id h;
        h.reset();
        auto const root = h.make("ok"sv);
        h.push_id("view1"sv);
        auto const in_view1 = h.make("ok"sv);
        h.pop_id();
        h.push_id("view2"sv);
        auto const in_view2 = h.make("ok"sv);
        h.pop_id();

        CHECK(root != in_view1);
        CHECK(in_view1 != in_view2);
        CHECK(h.make("ok"sv) == root);
    }

    TEST_CASE("push returns scope id") {
        im_hash_id h;
        h.reset();
        auto const expected = h.make("view"sv);
        CHECK(h.push_id("view"sv) == expected);
    }

    TEST_CASE("int scope") {
        im_hash_id h;
        h.reset();
        h.push_id(1);
        auto const a = h.make("row"sv);
        h.pop_id();
        h.push_id(2);
        CHECK(h.make("row"sv) != a);
    }

    TEST_CASE("pop never removes root") {
        im_hash_id h;
        h.reset();
        auto const root = h.make("x"sv);
        h.pop_id();
        h.pop_id();
        CHECK(h.make("x"sv) == root);
    }

    TEST_CASE("nesting overflow throws") {
        im_hash_id h;
        h.reset(); // root takes 1 of 32 slots
        for (int i = 0; i < 31; ++i) {
            h.push_id(i);
        }
        CHECK_THROWS_AS(h.push_id(99), std::bad_alloc);
    }
}

} // namespace xxx::testing
