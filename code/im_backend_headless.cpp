// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "im_backend_headless.h"

#include <cassert>

#include "unicode.h"

namespace xxx {

im_backend_headless::im_backend_headless(im_vec2 size) {
  resize(size);
}

auto im_backend_headless::size() const -> im_vec2 {
  return size_;
}

auto im_backend_headless::now() const -> im_clock::time_point {
  return now_;
}

void im_backend_headless::poll_events(im_input& input) {
  for (auto const& event : events_) {
    switch (event.type) {
    case pending_event::type::key:
      input.add_key_event(event.key);
      break;
    case pending_event::type::character:
      input.add_character(event.ch);
      if (event.ch == ' ') {
        input.add_key_event(im_key_id::space);
      }
      break;
    case pending_event::type::mouse_pos:
      input.add_mouse_pos_event(event.pos);
      break;
    case pending_event::type::mouse_button:
      input.add_mouse_pos_event(event.pos);
      input.add_mouse_button_event(event.button, event.pos);
      break;
    }
  }
  events_.clear();
}

void im_backend_headless::clear(im_style const& style) {
  std::fill(back_.begin(), back_.end(), im_cell{.ch = ' ', .style = style});
}

void im_backend_headless::set_cell(int x, int y, std::uint32_t ch, im_style const& style) {
  if (x < 0 || y < 0 || x >= size_.x || y >= size_.y) {
    return;
  }
  back_[y * size_.x + x] = im_cell{.ch = ch, .style = style};
}

void im_backend_headless::present() {
  front_ = back_;
  ++frames_;
}

void im_backend_headless::push_key(im_key_id key) {
  events_.push_back({.type = pending_event::type::key, .key = key});
}

void im_backend_headless::push_char(std::uint32_t ch) {
  events_.push_back({.type = pending_event::type::character, .ch = ch});
}

void im_backend_headless::push_text(std::string_view text) {
  for (auto const ch : utf8_to_unicode(text)) {
    push_char(ch);
  }
}

void im_backend_headless::push_mouse_pos(im_vec2 pos) {
  events_.push_back({.type = pending_event::type::mouse_pos, .pos = pos});
}

void im_backend_headless::push_mouse_button(im_mouse_button_id button, im_vec2 pos) {
  events_.push_back({.type = pending_event::type::mouse_button, .button = button, .pos = pos});
}

void im_backend_headless::advance_time(std::chrono::milliseconds delta) noexcept {
  now_ += delta;
}

void im_backend_headless::resize(im_vec2 size) {
  assert(size.x >= 0 && size.y >= 0);
  size_ = size;
  auto const blank = im_cell{.ch = ' ', .style = {}};
  back_.assign(std::size_t(size.x) * std::size_t(size.y), blank);
  front_ = back_;
}

auto im_backend_headless::cell(int x, int y) const -> im_cell const& {
  assert(x >= 0 && y >= 0 && x < size_.x && y < size_.y);
  return front_[y * size_.x + x];
}

auto im_backend_headless::line(int y) const -> std::string {
  auto result = std::string();
  // walk like a terminal: wide char advances cursor by two cells
  for (int x = 0; x < size_.x;) {
    auto const ch = front_[y * size_.x + x].ch;
    result += unicode_to_utf8(std::span<std::uint32_t const>(&ch, 1));
    x += char_width(ch);
  }
  result.erase(result.find_last_not_of(' ') + 1);
  return result;
}

auto im_backend_headless::screen() const -> std::string {
  auto result = std::string();
  for (int y = 0; y < size_.y; ++y) {
    if (y > 0) {
      result += '\n';
    }
    result += line(y);
  }
  result.erase(result.find_last_not_of('\n') + 1);
  return result;
}

} // namespace xxx
