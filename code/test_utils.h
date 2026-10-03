// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <format>

#include <doctest/doctest.h>

#include "im_color.h"
#include "im_rect.h"
#include "im_vec2.h"

// pretty-print library types in doctest failure messages

template <>
struct doctest::StringMaker<xxx::im_vec2> {
  static auto convert(xxx::im_vec2 const& v) -> doctest::String {
    return std::format("{}", v).c_str();
  }
};

template <>
struct doctest::StringMaker<xxx::im_rect> {
  static auto convert(xxx::im_rect const& r) -> doctest::String {
    return std::format("{}", r).c_str();
  }
};

template <>
struct doctest::StringMaker<xxx::im_color> {
  static auto convert(xxx::im_color const& c) -> doctest::String {
    return std::format("0x{:06x}", c.value).c_str();
  }
};
