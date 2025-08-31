// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx2.h"

#include <span>
#include <stdexcept>
#include <vector>

#include "xxx_internal.h"

namespace xxx {
namespace {

im_context ctx;

} // namespace

namespace v2 {

namespace draw {

void point(int x, int y, im_char ch, im_style const& style) noexcept {
  ::tb_set_cell(
      x, y, static_cast<std::uint32_t>(ch), static_cast<uintattr_t>(style.fg), static_cast<uintattr_t>(style.bg));
}

void hline(int x, int y, int length, im_char ch, im_style const& style = {}) noexcept {
  for (int pos = x, end = pos + length; pos < end; ++pos) {
    point(pos, y, ch, style);
  }
}

void vline(int x, int y, int length, im_char ch, im_style const& style = {}) noexcept {
  for (int pos = y, end = pos + length; pos < end; ++pos) {
    point(x, pos, ch, style);
  }
}

void text(int x, int y, std::span<im_char const> text, im_style const& style) noexcept {
  for (auto const ch : text) {
    point(x++, y, ch, style);
  }
}

} // namespace draw

namespace utf8 {

[[nodiscard]] auto utf8_to_unicode(std::string_view input) -> std::span<im_char const> {
  thread_local std::vector<im_char> cache;

  cache.resize(input.size());

  char const* begin = input.data();
  char const* end = begin + input.size();
  std::size_t pos = 0;

  while (begin < end) {
    if (*begin == '\0') {
      break;
    }
    auto const length = ::tb_utf8_char_length(*begin);
    if (begin + length > end) [[unlikely]] {
      break;
    }
    ::tb_utf8_char_to_unicode((std::uint32_t*)&cache[pos++], begin);
    begin += length;
  }

  return std::span(cache.data(), pos);
}

[[nodiscard]] auto utf8_to_unicode(char const* input) -> std::span<im_char const> {
  // TODO: optimize
  return utf8_to_unicode(std::string_view(input));
}

} // namespace utf8

namespace layout {

auto reserve_space(int height) noexcept -> im_rect {
  // FIXME
  (void)height;
  return {};
}

} // namespace layout

void init() {
  // TODO: return std::expected?

  // init termbox2 library
  if (auto const rc = ::tb_init(); rc != TB_OK) {
    throw std::runtime_error(::tb_strerror(rc));
  }
  ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
}

void shutdown() {
  ::tb_shutdown();
}

void poll_events(std::chrono::milliseconds timeout) {
  auto const start = clock::now();
  auto const expiration = start + timeout;

  ::tb_event event;
  while (true) {
    auto const rc = ::tb_peek_event(&event, static_cast<int>(timeout.count()));
    if (rc == TB_OK) {
      switch (event.type) {
      case TB_EVENT_KEY: {
        if (event.ch > 0) {
          // input text
        } else if (event.key > 0) {
          // input key
        }
      } break;
      case TB_EVENT_MOUSE: {
      } break;
      case TB_EVENT_RESIZE: {
      } break;
      default:
        break;
      };
    } else if (rc == TB_ERR_NO_EVENT) {
      // nothing to do
    } else if (rc == TB_ERR_POLL) {
      if (::tb_last_errno() != EINTR) {
        throw std::runtime_error(::tb_strerror(rc));
      }
    }

    auto const now = clock::now();
    if (now >= expiration) {
      break;
    }

    timeout = std::chrono::duration_cast<std::chrono::milliseconds>(expiration - now);
  }

  // what next
}

auto is_key_pressed(im_key key) -> bool {
  // TODO: fix me
  return false;
}

void new_frame() {
  ctx.layouts_stack.resize(1);

  auto top_level_layout = ctx.layouts_stack.back();
  top_level_layout.type = im_layout_type::container;
  top_level_layout.pos = {0, 0};
  top_level_layout.size = {::tb_width(), ::tb_height()};
  top_level_layout.filled = {0, 0};
  top_level_layout.columns = 0;
  top_level_layout.column = 0;

  ::tb_clear();
}

void render() {
  ::tb_present();
}

void label(char const* text) {
  // TODO
  draw::text(5, 5, utf8::utf8_to_unicode(text), {TB_DEFAULT, TB_DEFAULT});
}

} // namespace v2
} // namespace xxx
