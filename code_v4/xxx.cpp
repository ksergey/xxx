
// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cassert>
#include <format>
#include <functional>
#include <iterator>
#include <ranges>
#include <string_view>

#include "im_context.h"

#if 0
#include <print>
namespace xxx {

template <typename... Ts>
void debug(std::format_string<Ts...> fmt, Ts&&... args) {
  FILE* file = ::fopen("debug.txt", "a");
  if (!file) {
    return;
  }
  std::print(file, fmt, std::forward<Ts>(args)...);
  std::print(file, "\n");
  ::fclose(file);
}

} // namespace xxx
#endif

namespace xxx {

namespace {

[[nodiscard]] constexpr auto get_shorcut_label(im_key_id shortcut) noexcept -> std::string_view {
  using namespace std::string_view_literals;

  switch (shortcut) {
  case im_key_id::ctrl_a:
    return "c-a"sv;
  case im_key_id::ctrl_b:
    return "c-b"sv;
  case im_key_id::ctrl_c:
    return "c-c"sv;
  case im_key_id::ctrl_d:
    return "c-d"sv;
  case im_key_id::ctrl_e:
    return "c-e"sv;
  case im_key_id::ctrl_f:
    return "c-f"sv;
  case im_key_id::ctrl_g:
    return "c-g"sv;
  case im_key_id::ctrl_j:
    return "c-j"sv;
  case im_key_id::ctrl_k:
    return "c-k"sv;
  case im_key_id::ctrl_n:
    return "c-n"sv;
  case im_key_id::ctrl_o:
    return "c-o"sv;
  case im_key_id::ctrl_p:
    return "c-p"sv;
  case im_key_id::ctrl_q:
    return "c-q"sv;
  case im_key_id::ctrl_r:
    return "c-r"sv;
  case im_key_id::ctrl_s:
    return "c-s"sv;
  case im_key_id::ctrl_t:
    return "c-t"sv;
  case im_key_id::ctrl_u:
    return "c-u"sv;
  case im_key_id::ctrl_v:
    return "c-v"sv;
  case im_key_id::ctrl_w:
    return "c-w"sv;
  case im_key_id::ctrl_x:
    return "c-x"sv;
  case im_key_id::ctrl_y:
    return "c-y"sv;
  case im_key_id::ctrl_z:
    return "c-z"sv;
  default:
    return "";
  }
}

void handle_terminal_key_event(::tb_event const& event) {
  switch (event.key) {
  case TB_KEY_BACKSPACE:
    return g_ctx->input.add_key_event(im_key_id::backspace);
  case TB_KEY_BACKSPACE2:
    return g_ctx->input.add_key_event(im_key_id::backspace2);
  case TB_KEY_DELETE:
    return g_ctx->input.add_key_event(im_key_id::del);
  case TB_KEY_TAB:
    return g_ctx->input.add_key_event(im_key_id::tab);
  case TB_KEY_ENTER:
    return g_ctx->input.add_key_event(im_key_id::enter);
  case TB_KEY_ESC:
    return g_ctx->input.add_key_event(im_key_id::esc);
  case TB_KEY_SPACE:
    return g_ctx->input.add_key_event(im_key_id::space);
  case TB_KEY_HOME:
    return g_ctx->input.add_key_event(im_key_id::home);
  case TB_KEY_END:
    return g_ctx->input.add_key_event(im_key_id::end);
  case TB_KEY_ARROW_UP:
    return g_ctx->input.add_key_event(im_key_id::arrow_up);
  case TB_KEY_ARROW_DOWN:
    return g_ctx->input.add_key_event(im_key_id::arrow_down);
  case TB_KEY_ARROW_LEFT:
    return g_ctx->input.add_key_event(im_key_id::arrow_left);
  case TB_KEY_ARROW_RIGHT:
    return g_ctx->input.add_key_event(im_key_id::arrow_right);
  case TB_KEY_CTRL_A:
    return g_ctx->input.add_key_event(im_key_id::ctrl_a);
  case TB_KEY_CTRL_B:
    return g_ctx->input.add_key_event(im_key_id::ctrl_b);
  case TB_KEY_CTRL_C:
    return g_ctx->input.add_key_event(im_key_id::ctrl_c);
  case TB_KEY_CTRL_D:
    return g_ctx->input.add_key_event(im_key_id::ctrl_d);
  case TB_KEY_CTRL_E:
    return g_ctx->input.add_key_event(im_key_id::ctrl_e);
  case TB_KEY_CTRL_F:
    return g_ctx->input.add_key_event(im_key_id::ctrl_f);
  case TB_KEY_CTRL_G:
    return g_ctx->input.add_key_event(im_key_id::ctrl_g);
  case TB_KEY_CTRL_J:
    return g_ctx->input.add_key_event(im_key_id::ctrl_j);
  case TB_KEY_CTRL_K:
    return g_ctx->input.add_key_event(im_key_id::ctrl_k);
  case TB_KEY_CTRL_N:
    return g_ctx->input.add_key_event(im_key_id::ctrl_n);
  case TB_KEY_CTRL_O:
    return g_ctx->input.add_key_event(im_key_id::ctrl_o);
  case TB_KEY_CTRL_P:
    return g_ctx->input.add_key_event(im_key_id::ctrl_p);
  case TB_KEY_CTRL_Q:
    return g_ctx->input.add_key_event(im_key_id::ctrl_q);
  case TB_KEY_CTRL_R:
    return g_ctx->input.add_key_event(im_key_id::ctrl_r);
  case TB_KEY_CTRL_S:
    return g_ctx->input.add_key_event(im_key_id::ctrl_s);
  case TB_KEY_CTRL_T:
    return g_ctx->input.add_key_event(im_key_id::ctrl_t);
  case TB_KEY_CTRL_U:
    return g_ctx->input.add_key_event(im_key_id::ctrl_u);
  case TB_KEY_CTRL_V:
    return g_ctx->input.add_key_event(im_key_id::ctrl_v);
  case TB_KEY_CTRL_W:
    return g_ctx->input.add_key_event(im_key_id::ctrl_w);
  case TB_KEY_CTRL_X:
    return g_ctx->input.add_key_event(im_key_id::ctrl_x);
  case TB_KEY_CTRL_Y:
    return g_ctx->input.add_key_event(im_key_id::ctrl_y);
  case TB_KEY_CTRL_Z:
    return g_ctx->input.add_key_event(im_key_id::ctrl_z);
  default:
    break;
  }
}

void handle_terminal_mouse_event(::tb_event const& event) {
  g_ctx->input.add_mouse_pos_event(im_vec2(event.x, event.y));

  if (event.key > 0) {
    switch (event.key) {
    case TB_KEY_MOUSE_LEFT:
      return g_ctx->input.add_mouse_button_event(im_mouse_button_id::left, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_RIGHT:
      return g_ctx->input.add_mouse_button_event(im_mouse_button_id::right, im_vec2(event.x, event.y));
    case TB_KEY_MOUSE_MIDDLE:
      return g_ctx->input.add_mouse_button_event(im_mouse_button_id::middle, im_vec2(event.x, event.y));
    default:
      break;
    }
  }
}

template <typename OutputIt>
auto to_unicode(std::string_view input, OutputIt first) -> OutputIt {
  char const* begin = input.data();
  char const* end = begin + input.size();

  while (begin < end) {
    if (*begin == '\0') {
      break;
    }
    auto const length = ::tb_utf8_char_length(*begin);
    if (begin + length > end) [[unlikely]] {
      break;
    }
    std::uint32_t ch;
    ::tb_utf8_char_to_unicode(&ch, begin);
    *first++ = ch;

    begin += length;
  }

  return first;
}

[[nodiscard]] auto to_unicode(std::string_view input) noexcept -> std::span<std::uint32_t const> {
  auto buffer = g_ctx->allocator.allocate<std::uint32_t>(input.size());
  if (!buffer) [[unlikely]] {
    return std::span<std::uint32_t const>();
  }

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
    ::tb_utf8_char_to_unicode(&buffer[pos++], begin);
    begin += length;
  }

  return std::span(buffer, pos);
}

[[nodiscard]] constexpr auto unicode_codepoint_length(std::uint32_t c) noexcept -> std::size_t {
  if (c < 0x80) {
    return 1;
  } else if (c < 0x800) {
    return 2;
  } else if (c < 0x10000) {
    return 3;
  } else if (c < 0x200000) {
    return 4;
  } else if (c < 0x4000000) {
    return 5;
  } else {
    return 6;
  }
}

#if 0
[[nodiscard]] auto to_utf8(std::span<std::uint32_t const> input) -> std::string_view {
  auto const output_length =
      std::ranges::fold_left(std::views::transform(input, unicode_codepoint_length), 0, std::plus{});

  auto buffer = g_ctx->allocator.allocate<char>(output_length);
  if (!buffer) [[unlikely]] {
    return std::string_view();
  }

  auto offset = std::size_t(0);
  for (auto ch : input) {
    offset += ::tb_utf8_unicode_to_char(buffer + offset, ch);
  }

  return std::string_view(buffer, offset);
}
#endif

template <typename OutputIt>
auto to_utf8(std::span<std::uint32_t const> input, OutputIt first) -> OutputIt {
  char codepoint[7];
  for (auto ch : input) {
    auto const length = ::tb_utf8_unicode_to_char(codepoint, ch);
    for (auto c : std::string_view(codepoint, length)) {
      *first++ = c;
    }
  }

  return first;
}

[[nodiscard]] auto get_style_bg(im_color_id bg_id) noexcept -> im_style {
  return im_style(im_color(), g_ctx->theme.get_color(bg_id));
}

[[nodiscard]] auto get_style(im_color_id fg_id, im_color_id bg_id) noexcept -> im_style {
  return g_ctx->theme.get_style(fg_id, bg_id);
}

} // namespace

void init() {
  if (g_ctx) {
    delete g_ctx;
  }
  g_ctx = new im_context;
  g_ctx->allocator.reserve(2 * 1024 * 1024);

  // init termbox2 library
  if (auto const rc = ::tb_init(); rc != TB_OK) {
    throw std::runtime_error(::tb_strerror(rc));
  }

  ::tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
  ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
  ::tb_sendf("\x1b[?%d;%dh", 1003, 1006);

  g_ctx->last_frame_time = im_clock::now();
}

void shutdown() {
  ::tb_sendf("\x1b[?%d;%dl", 1003, 1006);
  ::tb_shutdown();

  delete g_ctx;
  g_ctx = nullptr;
}

void process_input_events() {
  g_ctx->input.reset();

  ::tb_event event;

  auto do_peek_events = true;
  while (do_peek_events) {
    auto const rc = ::tb_peek_event(&event, 0);
    if (rc == TB_OK) {
      switch (event.type) {
      case TB_EVENT_KEY: {
        if (event.ch > 0) {
          g_ctx->input.add_character(event.ch);
          if (event.ch == ' ') {
            g_ctx->input.add_key_event(im_key_id::space);
          }
        } else if (event.key > 0) {
          handle_terminal_key_event(event);
        }
      } break;
      case TB_EVENT_MOUSE: {
        handle_terminal_mouse_event(event);
      } break;
      case TB_EVENT_RESIZE: {
      } break;
      default:
        break;
      }
    } else if (rc == TB_ERR_NO_EVENT) {
      do_peek_events = false;
    } else if (rc == TB_ERR_POLL) {
      // handle poll error
      if (::tb_last_errno() != EINTR) {
        throw std::runtime_error(::tb_strerror(rc));
      }
    }
  }

  auto& view = g_ctx->view;
  auto& widget = g_ctx->widget;
  if (is_key_pressed(im_key_id::tab)) {
    if (widget.next_id != im_id()) {
      widget.active_id = widget.next_id;
    } else if (widget.first_id != im_id()) {
      widget.active_id = widget.first_id;
    }
  }

  if (view.force_next_id != im_id()) {
    if (view.active_id != view.force_next_id) {
      view.active_id = view.force_next_id;
      // reset active widget_id on active_id changed
      widget.active_id = im_id();
    }
  }
}

void new_frame() {
  assert(g_ctx);

  g_ctx->allocator.reset();

  // TODO: frame delta

  auto const screen_rect = im_rect(0, 0, ::tb_width() - 1, ::tb_height() - 1);

  g_ctx->hash_id.reset();
  g_ctx->theme.reset();
  g_ctx->layout.reset(screen_rect);

  g_ctx->view.current_title = "N/A";
  g_ctx->view.current_id = im_id();
  g_ctx->view.current_flags = 0;
  g_ctx->view.force_next_id = im_id();
  g_ctx->view.active = false;

  g_ctx->widget.current_id = im_id();
  g_ctx->widget.first_id = im_id();
  g_ctx->widget.next_id = im_id();
  g_ctx->widget.active = false;
  g_ctx->widget.pressed = false;

  auto const now = im_clock::now();
  g_ctx->elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_ctx->last_frame_time).count() * 0.001f;
  g_ctx->last_frame_time = now;

  g_ctx->renderer.set_clear_color(g_ctx->theme.get_style(im_color_id::text, im_color_id::background));
  g_ctx->renderer.start_new_frame(screen_rect);
}

void render() {
  assert(g_ctx);

  g_ctx->renderer.render();
}

void debug() {
  g_ctx->renderer.cmd_draw_rect(im_rect(2, 2, 6, 6), im_style(0x3366ff_c));
  g_ctx->renderer.cmd_draw_rect(im_rect(8, 8, 9, 9), im_style(0x33ff66_c));
  auto text = to_unicode("hello world 1234");
  auto rect = g_ctx->renderer.clip_rect().crop(10);
  g_ctx->renderer.cmd_draw_text_in_rect(
      rect, text, im_style(0xffee33_c, 0x332211), im_halign::center, im_valign::center);
}

auto get_screen_rect() -> im_rect {
  return im_rect(0, 0, ::tb_width() - 1, ::tb_height() - 1);
}

auto is_key_pressed(im_key_id id) -> bool {
  return g_ctx->input.is_key_pressed(id);
}

void set_default_color(im_color_id id, im_color color) {
  g_ctx->theme.set_default_color(id, color);
}

void push_color(im_color_id id, im_color color) {
  g_ctx->theme.push_color(id, color);
}

void pop_color(std::size_t cnt) {
  g_ctx->theme.pop_color(cnt);
}

void layout_row_begin(std::size_t columns) {
  if (columns == 0) [[unlikely]] {
    assert(false && "layout_row_begin(...) invalid argument");
    return;
  }

  g_ctx->layout.cursor_state_stack.push_back(g_ctx->layout.cursor);

  auto& container_layout = g_ctx->layout.layout_state_stack.back();
  auto& row_layout = g_ctx->layout.layout_state_stack.emplace_back();

  row_layout.type = im_layout_type::row;
  row_layout.rect.min = g_ctx->layout.cursor;
  row_layout.rect.max = im_vec2(container_layout.rect.max.x, g_ctx->layout.cursor.y);
  row_layout.row = im_layout_data_row{.columns = int(columns), .index = 0, .cursor_max_y = g_ctx->layout.cursor.y};

  // at this point
  // row_layout.rect.max.x != row_layout.rect.min.x -> layout width set
  // row_layout.rect.max.y == row_layout.rect.min.y -> layout height unset (dynamic)
}

void layout_row_push(float ratio_or_width) {
  ratio_or_width = std::max<float>(0.0f, ratio_or_width);

  // offset position x since previous column
  int offset_min_x = 0;
  if (auto& column_layout = g_ctx->layout.layout_state_stack.back(); column_layout.type == im_layout_type::column) {
    offset_min_x = column_layout.rect.max.x + 1;
    g_ctx->layout.layout_state_stack.pop_back();
  }

  auto& row_layout = g_ctx->layout.layout_state_stack.back();
  if (row_layout.type != im_layout_type::row) [[unlikely]] {
    assert(false && "layout_row_push(...) out of order");
    return;
  }
  if (row_layout.row.index == row_layout.row.columns) [[unlikely]] {
    assert(false && "layout_row_push(...) max columns reached");
    return;
  }

  // adjust to row layout in case of no previous columns
  offset_min_x = std::max(offset_min_x, row_layout.rect.min.x);
  // update cursor max y
  row_layout.row.cursor_max_y = std::max(row_layout.row.cursor_max_y, g_ctx->layout.cursor.y);

  auto const width =
      ratio_or_width > 1.0f ? int(ratio_or_width) : int(std::ceil(ratio_or_width * row_layout.rect.width()));
  auto const offset_max_x = std::min<int>(offset_min_x + width - 1, row_layout.rect.max.x);

  auto& column_layout = g_ctx->layout.layout_state_stack.emplace_back();
  column_layout.type = im_layout_type::column;
  column_layout.rect.min = im_vec2(offset_min_x, row_layout.rect.min.y);
  column_layout.rect.max = im_vec2(offset_max_x, row_layout.rect.min.y);
  column_layout.none = {};

  row_layout.row.index++;

  g_ctx->layout.cursor = column_layout.rect.min;
}

void layout_row_end() {
  if (auto& column_layout = g_ctx->layout.layout_state_stack.back(); column_layout.type == im_layout_type::column) {
    g_ctx->layout.layout_state_stack.pop_back();
  }

  auto& row_layout = g_ctx->layout.layout_state_stack.back();
  if (row_layout.type != im_layout_type::row) [[unlikely]] {
    assert(false && "layout_row_end(...) out of order");
    return;
  }

  auto const cursor_max_y = std::max(row_layout.row.cursor_max_y, g_ctx->layout.cursor.y);
  g_ctx->layout.layout_state_stack.pop_back();

  g_ctx->layout.cursor = im_vec2(g_ctx->layout.cursor_state_stack.back().x, cursor_max_y);
  g_ctx->layout.cursor_state_stack.pop_back();
}

void view_begin(std::string_view name, int flags, im_key_id shortcut) {
  auto& view = g_ctx->view;
  if (view.current_id != im_id()) {
    assert(false && "view_begin(...) inside another view");
    return;
  }

  auto const [str, view_key] = g_ctx->hash_id.split_str_key(name);

  if (shortcut != im_key_id()) {
    view.current_title = std::format(" {} <{}> ", str, get_shorcut_label(shortcut));
  } else {
    view.current_title = std::format(" {} ", str);
  }
  view.current_id = g_ctx->hash_id.push_id(view_key);
  view.current_flags = flags;

  if (view.active_id == im_id()) {
    view.active_id = view.current_id;
  }
  view.active = (view.active_id == view.current_id);

  if (is_key_pressed(shortcut) && !view.active) {
    view.force_next_id = view.current_id;
  }
  // TODO: skip frame?

  // layout and visuals
  {
    auto& container_layout = g_ctx->layout.layout_state_stack.back();
    auto& layout = g_ctx->layout.layout_state_stack.emplace_back();

    auto const do_render_border = (im_view_flag_border == (flags & im_view_flag_border));
    auto const do_render_title = (im_view_flag_title == (flags & im_view_flag_title));
    auto const border = do_render_border ? 1 : 0;

    layout.type = im_layout_type::container;
    layout.rect.min = g_ctx->layout.cursor + im_vec2(border, border);
    layout.rect.max = im_vec2(container_layout.rect.max.x - border, g_ctx->layout.cursor.y);
    layout.container = im_layout_data_container{.border = border};

    g_ctx->layout.cursor_state_stack.push_back(g_ctx->layout.cursor);
    g_ctx->layout.cursor = layout.rect.min;

    if (!do_render_border && do_render_title) {
      auto const title_rect = g_ctx->layout.add_widget_item(1);
      auto const style = view.active ? g_ctx->theme.get_style(im_color_id::view_active_title, im_color_id::background)
                                     : g_ctx->theme.get_style(im_color_id::view_title, im_color_id::background);
      g_ctx->renderer.cmd_fill_rect(title_rect, ' ', style);
      g_ctx->renderer.cmd_draw_text_in_rect(
          title_rect, to_unicode(view.current_title), style, im_halign::center, im_valign::top);
    }
  }
}

void view_end() {
  auto& view = g_ctx->view;

  g_ctx->hash_id.pop_id();

  // layout and visuals
  {
    auto const do_render_border = (im_view_flag_border == (view.current_flags & im_view_flag_border));
    auto const do_render_title = (im_view_flag_title == (view.current_flags & im_view_flag_title));

    auto& layout = g_ctx->layout.layout_state_stack.back();
    if (layout.type != im_layout_type::container) [[unlikely]] {
      assert(false && "view_end(...) out of order");
      return;
    }
    auto const border = layout.container.border;

    // calculate panel whole size
    auto const panel_rect = im_rect(layout.rect.min - im_vec2(border, border),
        im_vec2(layout.rect.max.x + border, g_ctx->layout.cursor.y + border - 1));

    g_ctx->layout.layout_state_stack.pop_back();

    if (do_render_border) {
      auto const style = view.active ? g_ctx->theme.get_style(im_color_id::view_active_border, im_color_id::background)
                                     : g_ctx->theme.get_style(im_color_id::view_border, im_color_id::background);
      g_ctx->renderer.cmd_draw_rect(panel_rect, style);

      if (do_render_title) {
        auto const style = view.active ? g_ctx->theme.get_style(im_color_id::view_active_title, im_color_id::background)
                                       : g_ctx->theme.get_style(im_color_id::view_title, im_color_id::background);
        g_ctx->renderer.cmd_draw_text_in_rect(
            panel_rect, to_unicode(view.current_title), style, im_halign::center, im_valign::top);
      }
    }

    g_ctx->layout.cursor = im_vec2(g_ctx->layout.cursor_state_stack.back().x, g_ctx->layout.cursor.y + border);
    g_ctx->layout.cursor_state_stack.pop_back();
  }

  view.current_title = "N/A";
  view.current_id = im_id();
  view.current_flags = 0;
  view.active = false;
}

void panel_begin() {
  constexpr auto border = int(1);

  auto& container_layout = g_ctx->layout.layout_state_stack.back();
  auto& layout = g_ctx->layout.layout_state_stack.emplace_back();

  layout.type = im_layout_type::container;
  layout.rect.min = g_ctx->layout.cursor + im_vec2(border, border);
  layout.rect.max = im_vec2(container_layout.rect.max.x - border, g_ctx->layout.cursor.y);
  layout.container = im_layout_data_container{.border = border};

  g_ctx->layout.cursor_state_stack.push_back(g_ctx->layout.cursor);
  g_ctx->layout.cursor = layout.rect.min;
}

void panel_end() {
  auto& layout = g_ctx->layout.layout_state_stack.back();
  if (layout.type != im_layout_type::container) [[unlikely]] {
    assert(false && "panel_end(...) out of order");
    return;
  }
  auto const border = layout.container.border;

  // calculate panel whole size
  auto const panel_rect = im_rect(layout.rect.min - im_vec2(border, border),
      im_vec2(layout.rect.max.x + border, g_ctx->layout.cursor.y + border - 1));

  g_ctx->layout.layout_state_stack.pop_back();

  auto const style = g_ctx->theme.get_style(im_color_id::border, im_color_id::background);
  g_ctx->renderer.cmd_draw_rect(panel_rect, style);

  g_ctx->layout.cursor = im_vec2(g_ctx->layout.cursor_state_stack.back().x, g_ctx->layout.cursor.y + border);
  g_ctx->layout.cursor_state_stack.pop_back();
}

void label(std::string_view text) {
  auto const widget_rect = g_ctx->layout.add_widget_item(1);
  if (!g_ctx->renderer.is_visible(widget_rect)) {
    return;
  }
  auto const style = g_ctx->theme.get_style(im_color_id::text, im_color_id::background);
  g_ctx->renderer.cmd_fill_rect(widget_rect, ' ', style);
  g_ctx->renderer.cmd_draw_text_in_rect(widget_rect, to_unicode(text), style, im_halign::left, im_valign::top);
}

namespace internal {

// update widget.* properties
void common_focusable_behaviour(im_id widget_id) noexcept {
  auto const& view = g_ctx->view;
  auto& widget = g_ctx->widget;

  widget.current_id = widget_id;
  widget.pressed = false;
  widget.active = false;

  // focus staff
  if (view.active) {
    if (widget.active_id == im_id()) {
      widget.active_id = widget.current_id;
    }
    if (widget.current_id != widget.active_id && widget.next_id == im_id()) {
      widget.next_id = widget.current_id;
    }
    if (widget.first_id == im_id()) {
      widget.first_id = widget.current_id;
    }
    widget.active = (widget.active_id == widget.current_id);
    if (widget.active) {
      widget.next_id = im_id();
    }
  }
}

} // namespace internal

auto button(std::string_view label) -> bool {
  auto& widget = g_ctx->widget;

  auto const widget_rect = g_ctx->layout.add_widget_item(1);
  auto const [str, widget_key] = g_ctx->hash_id.split_str_key(label);

  internal::common_focusable_behaviour(g_ctx->hash_id.make(widget_key));

  if (widget.active) {
    if (is_key_pressed(im_key_id::space) || is_key_pressed(im_key_id::enter)) {
      widget.pressed = true;
    }
  }

  static constexpr auto fx_left_ch = std::uint32_t(L'[');
  static constexpr auto fx_right_ch = std::uint32_t(L']');

  if (g_ctx->renderer.is_visible(widget_rect)) {
    // fill background
    g_ctx->renderer.cmd_fill_rect(widget_rect, ' ',
        widget.active ? get_style_bg(im_color_id::button_active_background)
                      : get_style_bg(im_color_id::button_inactive_background));

    auto unicode_str = to_unicode(str);

    // label start pos
    auto const unicode_str_pos = widget_rect.min + im_vec2((widget_rect.width() - unicode_str.size()) / 2, 0);

    // draw label
    g_ctx->renderer.cmd_draw_text_at(unicode_str_pos, unicode_str,
        widget.active ? get_style(im_color_id::button_active_text, im_color_id::button_active_background)
                      : get_style(im_color_id::button_inactive_text, im_color_id::button_inactive_background));

    // draw left fx
    g_ctx->renderer.cmd_draw_text_at(unicode_str_pos - im_vec2(2, 0), std::span<std::uint32_t const>(&fx_left_ch, 1),
        widget.active ? get_style(im_color_id::button_active_fx, im_color_id::button_active_background)
                      : get_style(im_color_id::button_inactive_fx, im_color_id::button_inactive_background));

    // draw right fx
    g_ctx->renderer.cmd_draw_text_at(unicode_str_pos + im_vec2(2 + unicode_str.size() - 1, 0),
        std::span<std::uint32_t const>(&fx_right_ch, 1),
        widget.active ? get_style(im_color_id::button_active_fx, im_color_id::button_active_background)
                      : get_style(im_color_id::button_inactive_fx, im_color_id::button_inactive_background));
  }

  return widget.pressed;
}

auto text_input(std::string_view placeholder, std::string& input, [[maybe_unused]] int flags) -> bool {
  auto& widget = g_ctx->widget;
  auto& text_input = g_ctx->text_input;

  auto const widget_rect = g_ctx->layout.add_widget_item(1);
  auto const [str, widget_key] = g_ctx->hash_id.split_str_key(placeholder);

  internal::common_focusable_behaviour(g_ctx->hash_id.make(widget_key));

  if (widget.active) {
    // utf8_to_unicode(input, text_input.text);
    text_input.text.clear();
    to_unicode(input, std::back_inserter(text_input.text));

    if (text_input.active_id != widget.active_id) {
      text_input.active_id = widget.active_id;
      text_input.cursor_pos = text_input.text.size();
      text_input.scroll_offset = 0;
    }
    if (int const text_length = text_input.text.size(); text_input.cursor_pos > text_length) {
      text_input.cursor_pos = text_length;
    }
    if (is_key_pressed(im_key_id::enter)) {
      widget.pressed = true;
    }

    auto text_changed = false;
    for (auto const event : g_ctx->input.get_input_events()) {
      if (event.ch > 0) {
        text_input.text.insert(text_input.text.begin() + text_input.cursor_pos, event.ch);
        text_input.cursor_pos++;
        text_changed = true;
      } else {
        switch (event.key) {
        case im_key_id::backspace:
        case im_key_id::backspace2: {
          if (text_input.cursor_pos > 0) {
            text_input.cursor_pos--;
            text_input.text.erase(text_input.text.begin() + text_input.cursor_pos);
            text_changed = true;
          }
        } break;
        case im_key_id::del: {
          if (int const text_length = text_input.text.size(); text_input.cursor_pos < text_length) {
            text_input.text.erase(text_input.text.begin() + text_input.cursor_pos);
            text_changed = true;
          }
        } break;
        case im_key_id::arrow_left: {
          if (text_input.cursor_pos > 0) {
            text_input.cursor_pos--;
          }
        } break;
        case im_key_id::arrow_right: {
          if (int const text_length = text_input.text.size(); text_input.cursor_pos < text_length) {
            text_input.cursor_pos++;
          }
        } break;
        case im_key_id::home: {
          text_input.cursor_pos = 0;
          text_input.scroll_offset = 0;
        } break;
        case im_key_id::end: {
          text_input.cursor_pos = text_input.text.size();
        } break;
        case im_key_id::ctrl_w: {
          if (!text_input.text.empty()) {
            if (auto const text_length = int(text_input.text.size()); text_input.cursor_pos >= text_length) {
              // on end of input move cursor to last char
              text_input.cursor_pos = text_length - 1;
            } else if (!std::isblank(text_input.text[text_input.cursor_pos])) {
              // keep symbol under cursor if non blank
              text_input.cursor_pos--;
            }
            // drop blanks before cursor
            while (text_input.cursor_pos >= 0 && std::isblank(text_input.text[text_input.cursor_pos])) {
              text_input.text.erase(text_input.text.begin() + text_input.cursor_pos--);
            }
            // drop until blank
            while (text_input.cursor_pos >= 0 && !std::isblank(text_input.text[text_input.cursor_pos])) {
              text_input.text.erase(text_input.text.begin() + text_input.cursor_pos--);
            }
            if (text_input.cursor_pos < 0) {
              text_input.cursor_pos = 0;
            } else {
              text_input.cursor_pos += 1;
            }
            text_changed = true;
          }
        } break;
        default:
          break;
        }
      }
    }

    assert(text_input.cursor_pos <= (int)text_input.text.size());

    if (text_changed) {
      input.clear();
      to_utf8(text_input.text, std::back_inserter(input));
    }
  }

  if (g_ctx->renderer.is_visible(widget_rect)) {
    auto const prompt = to_unicode("> ");
    auto rect = widget_rect;

    if (widget.active) {
      // fill background
      g_ctx->renderer.cmd_fill_rect(rect, ' ', get_style_bg(im_color_id::input_active_background));
      // draw prompt
      g_ctx->renderer.cmd_draw_text_at(
          rect.min, prompt, get_style(im_color_id::input_active_prompt, im_color_id::input_active_background));
    } else {
      // fill background
      g_ctx->renderer.cmd_fill_rect(rect, ' ', get_style_bg(im_color_id::input_inactive_background));
      // draw prompt
      g_ctx->renderer.cmd_draw_text_at(
          rect.min, prompt, get_style(im_color_id::input_inactive_prompt, im_color_id::input_inactive_background));
    }

    rect.min += im_vec2(prompt.size(), 0);

    // "static" keywoard is required here
    static constexpr auto space_ch = std::uint32_t(' ');

    if (input.empty()) {
      auto const unicode_str = to_unicode(str);
      if (widget.active) {
        if (!unicode_str.empty()) {
          {
            auto const style =
                get_style(im_color_id::input_active_text, im_color_id::input_active_background).with_reverse();
            g_ctx->renderer.cmd_draw_text_at(rect.min, unicode_str.subspan(0, 1), style);
          }
          rect.min += im_vec2(1, 0);
          {
            auto const style = get_style(im_color_id::input_placeholder, im_color_id::input_active_background);
            g_ctx->renderer.cmd_draw_text_at(rect.min, unicode_str.subspan(1), style);
          }
        } else {
          auto const style =
              get_style(im_color_id::input_active_text, im_color_id::input_active_background).with_reverse();
          g_ctx->renderer.cmd_draw_text_at(rect.min, std::span<std::uint32_t const>(&space_ch, 1), style);
        }
      } else {
        if (!unicode_str.empty()) {
          auto const style = get_style(im_color_id::input_placeholder, im_color_id::input_inactive_background);
          g_ctx->renderer.cmd_draw_text_at(rect.min, unicode_str, style);
        }
      }
    } else {
      if (widget.active) {
        auto const style = g_ctx->theme.get_style(im_color_id::input_active_text, im_color_id::input_active_background);
        auto const cursor_style = style.with_reverse();
        g_ctx->renderer.cmd_fill_rect(rect, ' ', style);

        auto const display_width = rect.width();
        auto a_content = std::span<std::uint32_t const>(text_input.text);
        auto a_content_size = int(a_content.size());
        auto a_cursor_pos = text_input.cursor_pos;

        if (a_content_size + 1 > display_width) {
          // context is greater of widget rect
          constexpr auto step = int(3);

          auto const abs_cursor_pos = a_cursor_pos + 1 - text_input.scroll_offset;
          if (abs_cursor_pos < 0) {
            text_input.scroll_offset -= abs_cursor_pos + step;
          } else if (abs_cursor_pos > display_width) {
            text_input.scroll_offset += abs_cursor_pos - display_width + step;
          }
          text_input.scroll_offset = std::max<int>(text_input.scroll_offset, 0);

          a_content = a_content.subspan(text_input.scroll_offset);
          a_content_size = int(a_content.size());
          a_cursor_pos = a_cursor_pos - text_input.scroll_offset;
        }

        if (a_cursor_pos < a_content_size) {
          if (a_cursor_pos > 0) {
            g_ctx->renderer.cmd_draw_text_at(rect.min, a_content.subspan(0, a_cursor_pos), style);
          }
          g_ctx->renderer.cmd_draw_text_at(
              rect.min + im_vec2(a_cursor_pos, 0), a_content.subspan(a_cursor_pos, 1), cursor_style);
          if (a_cursor_pos + 1 < a_content_size) {
            g_ctx->renderer.cmd_draw_text_at(
                rect.min + im_vec2(a_cursor_pos + 1, 0), a_content.subspan(a_cursor_pos + 1), style);
          }
        } else {
          g_ctx->renderer.cmd_draw_text_at(rect.min, a_content, style);
          g_ctx->renderer.cmd_draw_text_at(
              rect.min + im_vec2(a_content_size, 0), std::span<std::uint32_t const>(&space_ch, 1), cursor_style);
        }
      } else {
        auto const style =
            g_ctx->theme.get_style(im_color_id::input_inactive_text, im_color_id::input_inactive_background);
        g_ctx->renderer.cmd_fill_rect(rect, ' ', style);
        g_ctx->renderer.cmd_draw_text_at(rect.min, to_unicode(input), style);
      }
    }
  }

  return widget.pressed;
}

namespace {

constexpr auto spinner_update_interval = 0.1f; // 100ms
constexpr auto spinner_glyphs = std::to_array<std::uint32_t>({L'⣽', L'⣻', L'⢿', L'⡿', L'⣟', L'⣯', L'⣷'});

// TODO:
constexpr auto progress_glyph = std::to_array<std::uint32_t>({L'⣿', L'⡇'});

} // namespace

void spinner(std::string_view text, float& step) {
  auto const widget_rect = g_ctx->layout.add_widget_item(1);

  // update step
  step += g_ctx->elapsed;
  auto const index = std::size_t(std::round(step / spinner_update_interval)) % spinner_glyphs.size();

  if (!g_ctx->renderer.is_visible(widget_rect)) {
    return;
  }

  auto const style = g_ctx->theme.get_style(im_color_id::text, im_color_id::background);
  g_ctx->renderer.cmd_fill_rect(widget_rect, ' ', style);
  g_ctx->renderer.cmd_draw_text_at(widget_rect.min, std::span<std::uint32_t const>(&spinner_glyphs[index], 1), style);
  g_ctx->renderer.cmd_draw_text_at(widget_rect.min + im_vec2(1, 0), to_unicode(text), style);
}

void progress(float const& value) {
  auto const widget_rect = g_ctx->layout.add_widget_item(1);

  if (!g_ctx->renderer.is_visible(widget_rect)) {
    return;
  }

  auto const adjusted_value = std::clamp(value, 0.0f, 100.0f);
  auto const progress_total_length = widget_rect.width();
  auto const progress_length = static_cast<int>(std::round((progress_total_length * adjusted_value) / 100.0f));
  auto const buffer = g_ctx->allocator.allocate<std::uint32_t>(progress_total_length);
  if (!buffer) [[unlikely]] {
    return;
  }
  auto const text = std::span<std::uint32_t>(buffer, static_cast<std::size_t>(progress_total_length));
  std::fill(std::fill_n(text.begin(), progress_length, progress_glyph[0]), text.end(), L' ');

  auto const style = g_ctx->theme.get_style(im_color_id::text, im_color_id::background);
  g_ctx->renderer.cmd_draw_text_at(widget_rect.min, text, style);
}

namespace {

constexpr std::array braille_pixel_map = {
    std::array{0x01, 0x08},
    std::array{0x02, 0x10},
    std::array{0x04, 0x20},
    std::array{0x40, 0x80},
};

constexpr auto braille_offset = std::uint32_t(0x2800);
constexpr auto braille_pixels_per_width = 2;
constexpr auto braille_pixels_per_height = 4;

} // namespace

auto canvas_begin(im_vec2 p_size) -> bool {
  auto& canvas = g_ctx->canvas;

  auto const width = static_cast<int>(std::ceil(p_size.x / braille_pixels_per_width));
  auto const height = static_cast<int>(std::ceil(p_size.y / braille_pixels_per_height));

  canvas.size = im_vec2(width, height);
  auto const data_size = width * height;
  if (auto const data = g_ctx->allocator.allocate<im_cell>(data_size); data) {
    canvas.data = std::span<im_cell>(data, data_size);
  } else {
    canvas.data = {};
  }

  canvas.rect = g_ctx->layout.add_widget_item(height);
  g_ctx->renderer.push_clip_rect(canvas.rect);

  if (!canvas.data.empty()) {
    auto const style = g_ctx->theme.get_style(im_color_id::text, im_color_id::background);
    std::fill(canvas.data.begin(), canvas.data.end(), im_cell{.ch = braille_offset, .style = style});
    g_ctx->renderer.cmd_fill_rect(canvas.rect, ' ', style);
  }

  return g_ctx->renderer.is_visible(canvas.rect);
}

void canvas_end() {
  auto& canvas = g_ctx->canvas;

  if (!canvas.data.empty()) {
    g_ctx->renderer.cmd_draw_surface(canvas.rect.min, canvas.size, canvas.data);
  }

  canvas.rect = {};
  canvas.size = {};
  canvas.data = {};

  g_ctx->renderer.pop_clip_rect();
}

void canvas_point(im_vec2 p_pos, im_color color) {
  auto& canvas = g_ctx->canvas;

  // adjust pos to cells
  auto const pos = im_vec2(p_pos.x / braille_pixels_per_width, p_pos.y / braille_pixels_per_height);
  if (pos.x < 0 || pos.y < 0 || pos.x >= canvas.size.x || pos.y >= canvas.size.y) {
    return;
  }

  auto const cell = canvas.data.data() + pos.y * canvas.size.x + pos.x;
  cell->ch |= braille_pixel_map[p_pos.y % braille_pixels_per_height][p_pos.x % braille_pixels_per_width];
  cell->style.fg = color;
}

} // namespace xxx
