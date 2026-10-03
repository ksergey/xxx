// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "xxx.h"

#include "unicode.h"

namespace xxx {

// only key or ch
struct im_input_event {
  im_key_id key = im_key_id();
  std::uint32_t ch = 0;
};

class im_input {
private:
  struct keyboard_state {
    static constexpr std::size_t max_keys = static_cast<std::size_t>(im_key_id::last);
    // initial capacity only: queue grows, so pasted text or a slow frame loses nothing
    static constexpr std::size_t input_reserve = 64;

    struct key_state {
      std::size_t clicked = 0;
    };

    std::array<key_state, max_keys> keys;
    std::vector<im_input_event> input_events;
  };

  struct mouse_state {
    static constexpr std::size_t max_buttons = static_cast<std::size_t>(im_mouse_button_id::last);

    struct button_state {
      std::size_t clicked;
      im_vec2 clicked_pos;
    };

    std::array<button_state, max_buttons> buttons;
    im_vec2 pos = im_vec2(-1, -1);
    im_vec2 prev = im_vec2(-1, -1);
    im_vec2 delta;
    int wheel = 0; // > 0 scroll down, < 0 scroll up
  };

  keyboard_state keyboard_;
  mouse_state mouse_;

public:
  im_input() {
    keyboard_.input_events.reserve(keyboard_state::input_reserve);
  }

  [[nodiscard]] auto is_key_pressed(im_key_id id) const noexcept -> bool {
    assert(id < im_key_id::last);
    return keyboard_.keys[static_cast<std::size_t>(id)].clicked > 0;
  }

  /// How many times key was pressed this frame (key repeat, slow frame)
  [[nodiscard]] auto key_press_count(im_key_id id) const noexcept -> int {
    assert(id < im_key_id::last);
    return static_cast<int>(keyboard_.keys[static_cast<std::size_t>(id)].clicked);
  }

  [[nodiscard]] auto get_input_events() const noexcept -> std::span<im_input_event const> {
    return keyboard_.input_events;
  }

  void add_key_event(im_key_id id) noexcept {
    assert(id < im_key_id::last);
    keyboard_.keys[static_cast<std::size_t>(id)].clicked++;
    keyboard_.input_events.push_back(im_input_event{.key = id});
  }

  void add_mouse_pos_event(im_vec2 const& pos) noexcept {
    mouse_.pos = pos;
    mouse_.delta = mouse_.pos - mouse_.prev;
  }

  void add_mouse_button_event(im_mouse_button_id id, im_vec2 const& pos) noexcept {
    assert(id < im_mouse_button_id::last);

    auto& button = mouse_.buttons[static_cast<std::size_t>(id)];
    button.clicked++;
    button.clicked_pos = pos;
    mouse_.delta = im_vec2(0, 0);
  }

  void add_mouse_wheel_event(int delta, im_vec2 const& pos) noexcept {
    mouse_.pos = pos;
    mouse_.wheel += delta;
  }

  /// Button pressed this frame
  [[nodiscard]] auto is_mouse_clicked(im_mouse_button_id id) const noexcept -> bool {
    assert(id < im_mouse_button_id::last);
    return mouse_.buttons[static_cast<std::size_t>(id)].clicked > 0;
  }

  /// Position of last press of the button this frame
  [[nodiscard]] auto mouse_clicked_pos(im_mouse_button_id id) const noexcept -> im_vec2 {
    assert(id < im_mouse_button_id::last);
    return mouse_.buttons[static_cast<std::size_t>(id)].clicked_pos;
  }

  /// Last known mouse position, (-1, -1) if unknown
  [[nodiscard]] auto mouse_pos() const noexcept -> im_vec2 {
    return mouse_.pos;
  }

  /// Accumulated wheel steps this frame: > 0 down, < 0 up
  [[nodiscard]] auto mouse_wheel() const noexcept -> int {
    return mouse_.wheel;
  }

  void add_character(std::uint32_t ch) noexcept {
    keyboard_.input_events.push_back(im_input_event{.ch = ch});
  }

  void add_characters_utf8(char const* str) noexcept {
    for (auto const ch : utf8_to_unicode(str)) {
      this->add_character(ch);
    }
  }

  /// Drop keyboard input of this frame (it was handled and must not reach other widgets)
  void consume_keyboard() noexcept {
    keyboard_.keys.fill(keyboard_state::key_state{.clicked = 0});
    keyboard_.input_events.clear();
  }

  void reset() noexcept {
    keyboard_.keys.fill(keyboard_state::key_state{.clicked = 0});
    keyboard_.input_events.clear(); // keeps capacity

    mouse_.buttons.fill(mouse_state::button_state{.clicked = 0, .clicked_pos = im_vec2(0, 0)});
    mouse_.prev = mouse_.pos;
    mouse_.delta = im_vec2(0, 0);
    mouse_.wheel = 0;
  }
};

} // namespace xxx
