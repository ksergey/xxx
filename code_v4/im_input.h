// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstdint>

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
    static constexpr std::size_t input_max_length = 16;

    struct key_state {
      std::size_t clicked = 0;
    };

    std::array<key_state, max_keys> keys;
    std::array<im_input_event, input_max_length> input_events;
    std::size_t input_events_length = 0;
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
  };

  keyboard_state keyboard_;
  mouse_state mouse_;

public:
  im_input() = default;

  [[nodiscard]] auto is_key_pressed(im_key_id id) const noexcept -> bool {
    assert(id < im_key_id::last);
    return keyboard_.keys[static_cast<std::size_t>(id)].clicked > 0;
  }

  [[nodiscard]] auto get_input_events() const noexcept -> std::span<im_input_event const> {
    return std::span<im_input_event const>(keyboard_.input_events.data(), keyboard_.input_events_length);
  }

  void add_key_event(im_key_id id) noexcept {
    assert(id < im_key_id::last);
    keyboard_.keys[static_cast<std::size_t>(id)].clicked++;
    if (keyboard_.input_events_length < keyboard_.input_events.size()) {
      keyboard_.input_events[keyboard_.input_events_length++] = im_input_event{.key = id};
    }
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

  void add_character(std::uint32_t ch) noexcept {
    if (keyboard_.input_events_length < keyboard_.input_events.size()) {
      keyboard_.input_events[keyboard_.input_events_length++] = im_input_event{.ch = ch};
    }
  }

  void add_characters_utf8(char const* str) noexcept {
    for (auto const ch : utf8_to_unicode(str)) {
      this->add_character(ch);
    }
  }

  void reset() noexcept {
    keyboard_.keys.fill(keyboard_state::key_state{.clicked = 0});
    keyboard_.input_events_length = 0;

    mouse_.buttons.fill(mouse_state::button_state{.clicked = 0, .clicked_pos = im_vec2(0, 0)});
    mouse_.prev = mouse_.pos;
    mouse_.delta = im_vec2(0, 0);
  }
};

} // namespace xxx
