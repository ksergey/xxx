// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <cstdint>

#include <doctest/doctest.h>

#include "im_allocator.h"

namespace xxx::testing {

TEST_SUITE("im_allocator") {

  TEST_CASE("default constructed returns null") {
    im_allocator a;
    CHECK(a.allocate<int>() == nullptr);
  }

  TEST_CASE("allocations are aligned") {
    im_allocator a(1024);
    REQUIRE(a.allocate<char>(1) != nullptr); // misalign cursor
    auto* d = a.allocate<double>();
    REQUIRE(d != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(d) % alignof(double) == 0);
    auto* p = a.allocate<16>(8);
    REQUIRE(p != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(p) % 16 == 0);
  }

  TEST_CASE("allocations do not overlap") {
    im_allocator a(256);
    auto* x = a.allocate<std::uint32_t>(4);
    auto* y = a.allocate<std::uint32_t>(4);
    REQUIRE(x != nullptr);
    REQUIRE(y != nullptr);
    CHECK(reinterpret_cast<std::byte*>(y) >= reinterpret_cast<std::byte*>(x + 4));
  }

  // regression: remaining space used to be computed as full capacity
  TEST_CASE("returns null when exhausted") {
    im_allocator a(64);
    int ok = 0;
    for (int i = 0; i < 10; ++i) {
      if (a.allocate<std::uint32_t>(4) != nullptr) {
        ++ok;
      }
    }
    CHECK(ok == 4);
  }

  TEST_CASE("exact fit then null") {
    im_allocator a(64);
    CHECK(a.allocate<std::byte>(64) != nullptr);
    CHECK(a.allocate<std::byte>(1) == nullptr);
  }

  TEST_CASE("failed request does not consume space") {
    im_allocator a(64);
    CHECK(a.allocate<std::byte>(65) == nullptr);
    CHECK(a.allocate<std::byte>(64) != nullptr);
  }

  TEST_CASE("reset reuses memory") {
    im_allocator a(64);
    auto* first = a.allocate<std::byte>(64);
    REQUIRE(first != nullptr);
    REQUIRE(a.allocate<std::byte>(1) == nullptr);
    a.reset();
    CHECK(a.allocate<std::byte>(64) == first);
  }

  TEST_CASE("reserve") {
    SUBCASE("grows capacity") {
      im_allocator a(16);
      CHECK(a.allocate<std::byte>(32) == nullptr);
      a.reserve(128);
      CHECK(a.allocate<std::byte>(128) != nullptr);
    }
    SUBCASE("smaller keeps buffer") {
      im_allocator a(128);
      a.reserve(16);
      CHECK(a.allocate<std::byte>(128) != nullptr);
    }
  }

  TEST_CASE("move transfers buffer") {
    im_allocator a(64);
    im_allocator b(std::move(a));
    CHECK(b.allocate<std::byte>(64) != nullptr);
    CHECK(a.allocate<std::byte>(1) == nullptr);
  }
}

} // namespace xxx::testing
