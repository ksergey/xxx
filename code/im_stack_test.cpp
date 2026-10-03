// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <new>
#include <numeric>

#include <doctest/doctest.h>

#include "im_stack.h"

namespace xxx::testing {

TEST_SUITE("im_stack") {

  TEST_CASE("push / pop") {
    im_stack<int> s(4);
    CHECK(s.empty());
    CHECK(s.capacity() == 4);

    s.push_back(1);
    s.emplace_back(2);
    CHECK(s.size() == 2);
    CHECK(s.front() == 1);
    CHECK(s.back() == 2);
    CHECK(s[1] == 2);

    s.pop_back();
    CHECK(s.size() == 1);
    CHECK(s.back() == 1);
  }

  TEST_CASE("emplace_back returns reference to new element") {
    im_stack<int> s(2);
    auto& v = s.emplace_back(5);
    v = 7;
    CHECK(s.back() == 7);
  }

  TEST_CASE("overflow throws") {
    im_stack<int> s(2);
    s.push_back(1);
    s.push_back(2);
    CHECK_THROWS_AS(s.push_back(3), std::bad_alloc);
    CHECK_THROWS_AS(s.emplace_back(3), std::bad_alloc);
    CHECK(s.size() == 2);
  }

  TEST_CASE("iteration") {
    im_stack<int> s(8);
    for (int i = 1; i <= 4; ++i) {
      s.push_back(i);
    }
    CHECK(std::accumulate(s.begin(), s.end(), 0) == 10);
  }

  TEST_CASE("clear keeps capacity") {
    im_stack<int> s(3);
    s.push_back(1);
    s.clear();
    CHECK(s.empty());
    CHECK(s.capacity() == 3);
  }

  TEST_CASE("resize") {
    im_stack<int> s(4);
    s.resize(3, 9);
    CHECK(s.size() == 3);
    CHECK(s[2] == 9);
    s.resize(1);
    CHECK(s.size() == 1);
    CHECK_THROWS_AS(s.resize(5), std::bad_alloc);
    CHECK_THROWS_AS(s.resize(5, 0), std::bad_alloc);
  }

  TEST_CASE("copy is deep") {
    im_stack<int> a(4);
    a.push_back(1);
    auto b = a;
    b.push_back(2);
    b[0] = 42;
    CHECK(a.size() == 1);
    CHECK(a[0] == 1);
    CHECK(b.size() == 2);
    CHECK(b.capacity() == 4);
  }

  TEST_CASE("copy assignment") {
    im_stack<int> a(4);
    a.push_back(3);
    im_stack<int> b(1);
    b = a;
    CHECK(b.capacity() == 4);
    CHECK(b.back() == 3);
  }

  TEST_CASE("self copy assignment") {
    im_stack<int> a(2);
    a.push_back(5);
    auto& ref = a;
    a = ref;
    CHECK(a.size() == 1);
    CHECK(a.back() == 5);
  }

  TEST_CASE("move assignment") {
    im_stack<int> a(4);
    a.push_back(1);
    im_stack<int> b(2);
    b.push_back(9);
    b = std::move(a);
    CHECK(b.capacity() == 4);
    CHECK(b.back() == 1);
    CHECK(a.capacity() == 0);
    CHECK(a.empty());
  }

  TEST_CASE("swap") {
    im_stack<int> a(1), b(3);
    a.push_back(1);
    b.push_back(2);
    b.push_back(3);
    swap(a, b);
    CHECK(a.size() == 2);
    CHECK(a.capacity() == 3);
    CHECK(b.size() == 1);
    CHECK(b.back() == 1);
  }

  TEST_CASE("copy of default constructed") {
    im_stack<int> a;
    auto b = a;
    CHECK(b.empty());
    CHECK(b.capacity() == 0);
  }

  TEST_CASE("move leaves source empty") {
    im_stack<int> a(4);
    a.push_back(1);
    auto b = std::move(a);
    CHECK(b.size() == 1);
    CHECK(a.size() == 0);
    CHECK(a.capacity() == 0);
  }
}

} // namespace xxx::testing
