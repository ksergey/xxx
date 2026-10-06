// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "ansi_input.h"

#include <charconv>
#include <cstdint>

#include "unicode.h"

namespace xxx {

namespace {

constexpr auto esc = '\x1b';
constexpr auto paste_end = std::string_view("\x1b[201~");
// longest sequence we wait for; anything longer without a final byte is garbage
constexpr auto max_sequence = std::size_t(64);

// first number of CSI parameters ("5;3" -> 5), default when absent
[[nodiscard]] auto first_param(std::string_view params, int fallback) noexcept -> int {
  auto value = fallback;
  std::from_chars(params.data(), params.data() + params.size(), value);
  return value;
}

void add_control(unsigned char b, im_input& input) {
  switch (b) {
  case 0x08:
    return input.add_key_event(im_key_id::backspace);
  case 0x09:
    return input.add_key_event(im_key_id::tab);
  case 0x0a:
    return input.add_key_event(im_key_id::ctrl_j);
  case 0x0d:
    return input.add_key_event(im_key_id::enter);
  case 0x7f:
    return input.add_key_event(im_key_id::backspace2);
  default: {
    // explicit table: im_key_id has no ctrl_h / ctrl_i / ctrl_m (backspace, tab, enter) and no ctrl_l
    static constexpr im_key_id ctrl_keys[] = {
        im_key_id::ctrl_a, im_key_id::ctrl_b, im_key_id::ctrl_c, im_key_id::ctrl_d, im_key_id::ctrl_e,
        im_key_id::ctrl_f, im_key_id::ctrl_g, im_key_id(), im_key_id(), im_key_id::ctrl_j, im_key_id::ctrl_k,
        im_key_id(), im_key_id(), im_key_id::ctrl_n, im_key_id::ctrl_o, im_key_id::ctrl_p, im_key_id::ctrl_q,
        im_key_id::ctrl_r, im_key_id::ctrl_s, im_key_id::ctrl_t, im_key_id::ctrl_u, im_key_id::ctrl_v,
        im_key_id::ctrl_w, im_key_id::ctrl_x, im_key_id::ctrl_y, im_key_id::ctrl_z};
    if (b >= 0x01 && b <= 0x1a && ctrl_keys[b - 0x01] != im_key_id()) {
      input.add_key_event(ctrl_keys[b - 0x01]);
    }
    break; // NUL (ctrl-space), ctrl-l, ctrl-\ ] ^ _: not supported
  }
  }
}

} // namespace

auto ansi_input_parser::feed(std::string_view bytes, im_input& input) -> bool {
  produced_ = false;
  buffer_.append(bytes);

  while (!buffer_.empty()) {
    if (in_paste_) {
      auto const end = buffer_.find(paste_end);
      auto length = (end == std::string::npos) ? buffer_.size() : end;
      if (end == std::string::npos) {
        // keep a possible beginning of the terminator for the next feed
        if (auto const e = buffer_.rfind(esc); e != std::string::npos && buffer_.size() - e < paste_end.size()) {
          length = e;
        }
      }
      auto const before = buffer_.size();
      handle_paste_bytes(std::string_view(buffer_).substr(0, length), input);
      auto const used = before - buffer_.size();
      if (end != std::string::npos && used == length) {
        buffer_.erase(0, paste_end.size());
        in_paste_ = false;
        continue;
      }
      break; // wait for more bytes
    }

    consumed_ = 0;
    if (parse_one(input) == result::incomplete) {
      if (buffer_.size() > max_sequence) {
        buffer_.erase(0, 1); // garbage: drop ESC and resync
        continue;
      }
      break;
    }
    buffer_.erase(0, consumed_);
  }
  return produced_;
}

auto ansi_input_parser::flush(im_input& input) -> bool {
  produced_ = false;
  if (in_paste_) {
    return false; // paste continues, its terminator will come
  }
  if (buffer_ == std::string_view("\x1b")) {
    input.add_key_event(im_key_id::esc);
    produced_ = true;
  }
  buffer_.clear();
  return produced_;
}

void ansi_input_parser::handle_paste_bytes(std::string_view bytes, im_input& input) {
  auto used = std::size_t(0);
  while (used < bytes.size()) {
    std::uint32_t ch = 0;
    auto const n = utf8_decode(bytes.substr(used), ch);
    if (n == 0) {
      if (bytes[used] == '\0') {
        ++used;
        continue;
      }
      break; // truncated codepoint: wait for the rest
    }
    used += n;
    if (ch == '\t' || ch == '\n' || ch == '\r') {
      ch = ' '; // pasted text is text: a newline must not press enter
    } else if (ch < 0x20 || ch == 0x7f) {
      continue;
    }
    input.add_character(ch);
    produced_ = true;
  }
  buffer_.erase(0, used);
}

auto ansi_input_parser::parse_one(im_input& input) -> result {
  auto const b = static_cast<unsigned char>(buffer_[0]);

  if (b == esc) {
    if (buffer_.size() == 1) {
      return result::incomplete; // lone ESC: decided by flush()
    }
    switch (buffer_[1]) {
    case '[':
      return parse_csi(input);
    case 'O': {
      // SS3: application cursor keys
      if (buffer_.size() < 3) {
        return result::incomplete;
      }
      switch (buffer_[2]) {
      case 'A':
        input.add_key_event(im_key_id::arrow_up), produced_ = true;
        break;
      case 'B':
        input.add_key_event(im_key_id::arrow_down), produced_ = true;
        break;
      case 'C':
        input.add_key_event(im_key_id::arrow_right), produced_ = true;
        break;
      case 'D':
        input.add_key_event(im_key_id::arrow_left), produced_ = true;
        break;
      case 'H':
        input.add_key_event(im_key_id::home), produced_ = true;
        break;
      case 'F':
        input.add_key_event(im_key_id::end), produced_ = true;
        break;
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
        // F1-F4 (xterm)
        input.add_key_event(im_key_id(int(im_key_id::f1) + (buffer_[2] - 'P'))), produced_ = true;
        break;
      default:
        break;
      }
      consumed_ = 3;
      return result::done;
    }
    case esc:
      // ESC ESC: first one is a key
      input.add_key_event(im_key_id::esc);
      produced_ = true;
      consumed_ = 1;
      return result::done;
    default:
      // Alt + char: modifier is not supported, keep the char
      consumed_ = 1;
      return result::done;
    }
  }

  if (b < 0x20 || b == 0x7f) {
    auto const before = input.get_input_events().size();
    add_control(b, input);
    produced_ = produced_ || input.get_input_events().size() != before;
    consumed_ = 1;
    return result::done;
  }

  std::uint32_t ch = 0;
  auto const n = utf8_decode(buffer_, ch);
  if (n == 0) {
    return result::incomplete; // truncated codepoint
  }
  input.add_character(ch);
  if (ch == ' ') {
    input.add_key_event(im_key_id::space);
  }
  produced_ = true;
  consumed_ = n;
  return result::done;
}

auto ansi_input_parser::parse_csi(im_input& input) -> result {
  // ESC [ params (0x30-0x3f)* intermediates (0x20-0x2f)* final (0x40-0x7e)
  auto i = std::size_t(2);
  while (i < buffer_.size()) {
    auto const c = static_cast<unsigned char>(buffer_[i]);
    if (c >= 0x40 && c <= 0x7e) {
      break;
    }
    if (c < 0x20 || c > 0x3f) {
      consumed_ = i; // broken sequence: drop what we have, resync on this byte
      return result::done;
    }
    if (++i > max_sequence) {
      // too long to be real: drop ESC, the rest is read as text.
      // Same limit as for an incomplete sequence in feed(), so the result doesn't depend on read sizes
      consumed_ = 1;
      return result::done;
    }
  }
  if (i == buffer_.size()) {
    return result::incomplete;
  }
  consumed_ = i + 1;

  auto const params = std::string_view(buffer_).substr(2, i - 2);
  auto const final = buffer_[i];
  auto const key = [&](im_key_id id) {
    input.add_key_event(id);
    produced_ = true;
  };

  if (!params.empty() && params[0] == '<' && (final == 'M' || final == 'm')) {
    // SGR mouse: <button;x;y, 1-based, M press / m release
    int values[3] = {0, 0, 0};
    auto p = params.data() + 1;
    auto const end = params.data() + params.size();
    for (auto& v : values) {
      p = std::from_chars(p, end, v).ptr;
      if (p < end && *p == ';') {
        ++p;
      }
    }
    auto const pos = im_vec2(values[1] - 1, values[2] - 1);
    auto const button = values[0];
    produced_ = true;
    if (button & 64) {
      input.add_mouse_wheel_event((button & 1) ? +1 : -1, pos);
    } else if ((button & 32) || final == 'm') {
      input.add_mouse_pos_event(pos); // motion (also drag) or release: not a click
    } else {
      static constexpr im_mouse_button_id buttons[] = {
          im_mouse_button_id::left, im_mouse_button_id::middle, im_mouse_button_id::right};
      input.add_mouse_pos_event(pos);
      if ((button & 3) < 3) {
        input.add_mouse_button_event(buttons[button & 3], pos);
      }
    }
    return result::done;
  }

  switch (final) {
  case 'A':
    key(im_key_id::arrow_up);
    break;
  case 'B':
    key(im_key_id::arrow_down);
    break;
  case 'C':
    key(im_key_id::arrow_right);
    break;
  case 'D':
    key(im_key_id::arrow_left);
    break;
  case 'H':
    key(im_key_id::home);
    break;
  case 'F':
    key(im_key_id::end);
    break;
  case 'Z':
    key(im_key_id::back_tab);
    break;
  case 'P':
  case 'Q':
  case 'R':
  case 'S':
    key(im_key_id(int(im_key_id::f1) + (final - 'P'))); // F1-F4 with modifiers: ESC [ 1 ; m P
    break;
  case '~':
    switch (first_param(params, 0)) {
    case 1:
    case 7:
      key(im_key_id::home);
      break;
    case 4:
    case 8:
      key(im_key_id::end);
      break;
    case 3:
      key(im_key_id::del);
      break;
    case 5:
      key(im_key_id::page_up);
      break;
    case 6:
      key(im_key_id::page_down);
      break;
    case 200:
      in_paste_ = true;
      break;
    default:
      // function keys: 11-15 F1-F5 (rxvt / linux console), 17-21 F6-F10, 23 F11, 24 F12; insert: ignored
      static constexpr struct {
        int code;
        im_key_id key;
      } function_keys[] = {{11, im_key_id::f1}, {12, im_key_id::f2}, {13, im_key_id::f3}, {14, im_key_id::f4},
          {15, im_key_id::f5}, {17, im_key_id::f6}, {18, im_key_id::f7}, {19, im_key_id::f8}, {20, im_key_id::f9},
          {21, im_key_id::f10}, {23, im_key_id::f11}, {24, im_key_id::f12}};
      for (auto const code = first_param(params, 0); auto const& f : function_keys) {
        if (f.code == code) {
          key(f.key);
        }
      }
      break;
    }
    break;
  default:
    break; // unknown sequence: consumed, ignored
  }
  return result::done;
}

} // namespace xxx
