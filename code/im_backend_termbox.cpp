// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#include <termbox2.h>

#include "base64.h"
#include "im_backend.h"
#include "unicode.h"

static_assert(sizeof(uintattr_t) == 8, "termbox2 must be built with TB_OPT_ATTR_W=64");

namespace xxx {

namespace {

void handle_terminal_key_event(::tb_event const& event, im_input& input) {
  switch (event.key) {
  case TB_KEY_BACKSPACE:
    return input.add_key_event(im_key_id::backspace);
  case TB_KEY_BACKSPACE2:
    return input.add_key_event(im_key_id::backspace2);
  case TB_KEY_DELETE:
    return input.add_key_event(im_key_id::del);
  case TB_KEY_TAB:
    return input.add_key_event(im_key_id::tab);
  case TB_KEY_BACK_TAB:
    return input.add_key_event(im_key_id::back_tab);
  case TB_KEY_ENTER:
    return input.add_key_event(im_key_id::enter);
  case TB_KEY_ESC:
    return input.add_key_event(im_key_id::esc);
  case TB_KEY_SPACE:
    return input.add_key_event(im_key_id::space);
  case TB_KEY_HOME:
    return input.add_key_event(im_key_id::home);
  case TB_KEY_END:
    return input.add_key_event(im_key_id::end);
  case TB_KEY_ARROW_UP:
    return input.add_key_event(im_key_id::arrow_up);
  case TB_KEY_ARROW_DOWN:
    return input.add_key_event(im_key_id::arrow_down);
  case TB_KEY_ARROW_LEFT:
    return input.add_key_event(im_key_id::arrow_left);
  case TB_KEY_ARROW_RIGHT:
    return input.add_key_event(im_key_id::arrow_right);
  case TB_KEY_PGUP:
    return input.add_key_event(im_key_id::page_up);
  case TB_KEY_PGDN:
    return input.add_key_event(im_key_id::page_down);
  case TB_KEY_CTRL_A:
    return input.add_key_event(im_key_id::ctrl_a);
  case TB_KEY_CTRL_B:
    return input.add_key_event(im_key_id::ctrl_b);
  case TB_KEY_CTRL_C:
    return input.add_key_event(im_key_id::ctrl_c);
  case TB_KEY_CTRL_D:
    return input.add_key_event(im_key_id::ctrl_d);
  case TB_KEY_CTRL_E:
    return input.add_key_event(im_key_id::ctrl_e);
  case TB_KEY_CTRL_F:
    return input.add_key_event(im_key_id::ctrl_f);
  case TB_KEY_CTRL_G:
    return input.add_key_event(im_key_id::ctrl_g);
  case TB_KEY_CTRL_J:
    return input.add_key_event(im_key_id::ctrl_j);
  case TB_KEY_CTRL_K:
    return input.add_key_event(im_key_id::ctrl_k);
  case TB_KEY_CTRL_N:
    return input.add_key_event(im_key_id::ctrl_n);
  case TB_KEY_CTRL_O:
    return input.add_key_event(im_key_id::ctrl_o);
  case TB_KEY_CTRL_P:
    return input.add_key_event(im_key_id::ctrl_p);
  case TB_KEY_CTRL_Q:
    return input.add_key_event(im_key_id::ctrl_q);
  case TB_KEY_CTRL_R:
    return input.add_key_event(im_key_id::ctrl_r);
  case TB_KEY_CTRL_S:
    return input.add_key_event(im_key_id::ctrl_s);
  case TB_KEY_CTRL_T:
    return input.add_key_event(im_key_id::ctrl_t);
  case TB_KEY_CTRL_U:
    return input.add_key_event(im_key_id::ctrl_u);
  case TB_KEY_CTRL_V:
    return input.add_key_event(im_key_id::ctrl_v);
  case TB_KEY_CTRL_W:
    return input.add_key_event(im_key_id::ctrl_w);
  case TB_KEY_CTRL_X:
    return input.add_key_event(im_key_id::ctrl_x);
  case TB_KEY_CTRL_Y:
    return input.add_key_event(im_key_id::ctrl_y);
  case TB_KEY_CTRL_Z:
    return input.add_key_event(im_key_id::ctrl_z);
  default:
    break;
  }
}

void handle_terminal_mouse_event(::tb_event const& event, im_input& input) {
  input.add_mouse_pos_event(im_vec2(event.x, event.y));

  // with any-event tracking (1003) dragging reports the held button on every move:
  // only the initial press is a click
  if (event.mod & TB_MOD_MOTION) {
    return;
  }

  if (event.key > 0) {
    switch (event.key) {
    case TB_KEY_MOUSE_LEFT:
      return input.add_mouse_button_event(im_mouse_button_id::left, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_RIGHT:
      return input.add_mouse_button_event(im_mouse_button_id::right, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_MIDDLE:
      return input.add_mouse_button_event(im_mouse_button_id::middle, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_WHEEL_UP:
      return input.add_mouse_wheel_event(-1, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_WHEEL_DOWN:
      return input.add_mouse_wheel_event(+1, im_vec2(event.x, event.y));
    default:
      break;
    }
  }
}

void handle_event(::tb_event const& event, im_input& input) {
  switch (event.type) {
  case TB_EVENT_KEY:
    if (event.ch > 0) {
      input.add_character(event.ch);
      if (event.ch == ' ') {
        input.add_key_event(im_key_id::space);
      }
    } else if (event.key > 0) {
      handle_terminal_key_event(event, input);
    }
    break;
  case TB_EVENT_MOUSE:
    handle_terminal_mouse_event(event, input);
    break;
  default:
    // TB_EVENT_RESIZE: nothing to store, caller redraws with new size
    break;
  }
}

// im_style -> termbox attributes (truecolor mode, TB_OPT_ATTR_W=64)
[[nodiscard]] auto to_tb_fg(im_style const& style) noexcept -> uintattr_t {
  auto result = uintattr_t(style.fg);
  auto const a = style.attrs;
  result |= (a & im_attr_bold) ? TB_BOLD : 0;
  result |= (a & im_attr_dim) ? TB_DIM : 0;
  result |= (a & im_attr_italic) ? TB_ITALIC : 0;
  result |= (a & im_attr_underline) ? TB_UNDERLINE : 0;
  result |= (a & im_attr_blink) ? TB_BLINK : 0;
  result |= (a & im_attr_reverse) ? TB_REVERSE : 0;
  result |= (a & im_attr_strikeout) ? TB_STRIKEOUT : 0;
  return result;
}

[[nodiscard]] auto to_tb_bg(im_style const& style) noexcept -> uintattr_t {
  return uintattr_t(style.bg);
}

} // namespace

class im_backend_termbox final : public im_backend {
public:
  im_backend_termbox() {
    if (auto const rc = ::tb_init(); rc != TB_OK) {
      throw std::runtime_error(::tb_strerror(rc));
    }
    ::tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
    ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
    // any-event mouse tracking + SGR extended coordinates
    ::tb_sendf("\x1b[?%d;%dh", 1003, 1006);
  }

  ~im_backend_termbox() override {
    ::tb_sendf("\x1b[?%d;%dl", 1003, 1006);
    ::tb_shutdown();
  }

  im_backend_termbox(im_backend_termbox const&) = delete;
  im_backend_termbox& operator=(im_backend_termbox const&) = delete;

  [[nodiscard]] auto size() const -> im_vec2 override {
    return im_vec2(::tb_width(), ::tb_height());
  }

  auto poll_events(im_input& input, std::chrono::milliseconds timeout) -> bool override {
    auto got_event = false;
    ::tb_event event;
    // block only for the first event, then drain whatever is queued
    for (auto first = true;; first = false) {
      auto const wait = first ? timeout : std::chrono::milliseconds(0);
      auto const rc = wait.count() < 0 ? ::tb_poll_event(&event)
                                       : ::tb_peek_event(&event, int(std::min<std::int64_t>(wait.count(), INT32_MAX)));
      if (rc == TB_OK) {
        got_event = true;
        handle_event(event, input);
      } else if (rc == TB_ERR_NO_EVENT) {
        return got_event;
      } else if (rc == TB_ERR_POLL) {
        if (::tb_last_errno() != EINTR) {
          throw std::runtime_error(::tb_strerror(rc));
        }
        // interrupted by a signal (SIGWINCH on resize): worth a new frame
        return true;
      } else {
        return got_event;
      }
    }
  }

  void clear(im_style const& style) override {
    ::tb_set_clear_attrs(to_tb_fg(style), to_tb_bg(style));
    ::tb_clear();
  }

  void set_cell(int x, int y, std::uint32_t ch, im_style const& style) override {
    ::tb_set_cell(x, y, ch, to_tb_fg(style), to_tb_bg(style));
  }

  void present() override {
    ::tb_present();
  }

  void set_clipboard(std::string_view text) override {
    // OSC 52: terminal puts text into system clipboard; works over ssh too.
    // Queued into output buffer, flushed with the next present().
    auto const sequence = osc52_sequence(text, std::getenv("TMUX") != nullptr);
    ::tb_send(sequence.data(), sequence.size());
  }
};

auto make_termbox_backend() -> std::unique_ptr<im_backend> {
  return std::make_unique<im_backend_termbox>();
}

} // namespace xxx
