// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>

#include "xxx.h"

#include "im_stack.h"

namespace xxx {

class im_theme {
private:
  static constexpr auto max_color_states = static_cast<std::size_t>(im_color_id::last);

  struct color_state {
    im_color_id id;
    im_color color;
  };

  im_stack<color_state> state_stack_ = im_stack<color_state>(32);
  std::array<im_color, max_color_states> colors_;

public:
  im_theme() {
    // https://coolors.co/palette/848c8e-435058-dcf763-bfb7b6-f1f2ee

    this->set_default_color(im_color_id::text, {});
    this->set_default_color(im_color_id::background, {});
    this->set_default_color(im_color_id::border, {});
    this->set_default_color(im_color_id::view_border, {});
    this->set_default_color(im_color_id::view_title, {});

    this->set_default_color(im_color_id::view_active_border, 0xdcf763_c);
    this->set_default_color(im_color_id::view_active_title, 0xdcf763_c);

    this->set_default_color(im_color_id::button_inactive_background, im_color());
    this->set_default_color(im_color_id::button_inactive_text, 0x848c8e_c);
    this->set_default_color(im_color_id::button_inactive_fx, 0xbfb7b6_c);
    this->set_default_color(im_color_id::button_active_background, im_color());
    this->set_default_color(im_color_id::button_active_text, 0xdcf763_c);
    this->set_default_color(im_color_id::button_active_fx, 0xdcf763_c);

    this->set_default_color(im_color_id::input_inactive_background, im_color());
    this->set_default_color(im_color_id::input_inactive_text, 0x848c8e_c);
    this->set_default_color(im_color_id::input_inactive_prompt, 0xf1f2ee_c);
    this->set_default_color(im_color_id::input_active_background, im_color());
    this->set_default_color(im_color_id::input_active_text, 0xdcf763_c);
    this->set_default_color(im_color_id::input_active_prompt, 0xdcf763_c);
    this->set_default_color(im_color_id::input_placeholder, 0x848c8e_c);
  }

  [[nodiscard]] auto get_style(im_color_id fg_id, im_color_id bg_id) const noexcept -> im_style {
    return im_style(this->get_color(fg_id), this->get_color(bg_id));
  }

  [[nodiscard]] auto get_color(im_color_id id) const noexcept -> im_color const& {
    assert(id < im_color_id::last);
    auto const index = static_cast<std::size_t>(id);
    return colors_[index];
  }

  void set_default_color(im_color_id id, im_color color) noexcept {
    assert(id < im_color_id::last);
    auto const index = static_cast<std::size_t>(id);
    colors_[index] = color;
  }

  void push_color(im_color_id id, im_color color) {
    assert(id < im_color_id::last);
    auto const index = static_cast<std::size_t>(id);
    state_stack_.emplace_back(id, std::exchange(colors_[index], color));
  }

  void pop_color(std::size_t cnt = 1) {
    while (state_stack_.size() > 0 && cnt-- > 0) {
      auto const index = static_cast<std::size_t>(state_stack_.back().id);
      colors_[index] = state_stack_.back().color;
      state_stack_.pop_back();
    }
  }

  void reset() noexcept {
    pop_color(99999);
  }
};

} // namespace xxx
