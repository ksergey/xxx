// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <array>
#include <variant>
#include <vector>

#include <termbox2.h>

#include "xxx2.h"

namespace xxx {

static_assert(std::is_same_v<uintattr_t, std::uint64_t>, "termbox2 invalid configuration");

// ------------------------------------
// basic containers
// ------------------------------------
template <typename T>
using im_vector = std::vector<T>;

template <typename T, std::size_t N>
using im_array = std::array<T, N>;

// ------------------------------------
// commands
// ------------------------------------
struct im_command_nop {};

struct im_command_hline {
  im_vec2 begin;
  int legnth;
  im_color color;
};

struct im_command_vline {
  im_vec2 begin;
  int legnth;
  im_color color;
};

struct im_command_rect {
  im_vec2 begin;
  im_vec2 end;
  im_color color;
};

struct im_command_rect_filled {
  im_vec2 begin;
  im_vec2 end;
  im_color color;
};

struct im_command_text {
  // TODO
};

using im_command = std::variant<im_command_nop, im_command_hline, im_command_vline, im_command_rect,
    im_command_rect_filled, im_command_text>;

// ------------------------------------
// input
// ------------------------------------
struct im_mouse_button {
  bool down;
  unsigned clicked;
  im_vec2 pos;
};

struct im_mouse {};

struct im_key {
  bool down;
  unsigned clicked;
};

struct im_keyboard {};

struct im_input {
  im_mouse mouse;
  im_keyboard keyboard;
};

// ------------------------------------
// context
// ------------------------------------
struct im_context {};

} // namespace xxx
