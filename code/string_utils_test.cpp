// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <array>
#include <string_view>

#include <doctest/doctest.h>

#include "string_utils.h"

namespace xxx::testing {

using namespace std::string_view_literals;

TEST_SUITE("string_utils") {

    TEST_CASE("substr string_view") {
        CHECK(substr("hello"sv, 1, 3) == "ell"sv);
        CHECK(substr("hello"sv, 2) == "llo"sv);
        CHECK(substr("hello"sv, 3, 100) == "lo"sv);
    }

    TEST_CASE("substr span") {
        auto data = std::to_array({1, 2, 3, 4, 5});
        auto const s = std::span<int>(data);

        auto const mid = substr(s, 1, 3);
        REQUIRE(mid.size() == 3);
        CHECK(mid[0] == 2);
        CHECK(mid[2] == 4);

        CHECK(substr(s, 3, 100).size() == 2);
        CHECK(substr(s, 5).empty());
        // out of range pos is clamped to empty instead of UB
        CHECK(substr(s, 10).empty());
    }
}

} // namespace xxx::testing
