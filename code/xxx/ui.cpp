// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#include "ui.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>

#include "impl/data.h"
#include "impl/draw.h"
#include "impl/utf8.h"

namespace xxx {

using namespace xxx::impl;

namespace {

context ctx;

}  // namespace

namespace detail {

XXX_ALWAYS_INLINE xxx::rect reserve_space(int height) noexcept {
  if (height < 0) {
    height = 0;
  }

  auto& layout = ctx.layout_stack.back();

  xxx::rect result;
  result.x = layout.pos.x;
  result.y = layout.pos.y + layout.filled_size.height;
  result.width = layout.size.width;
  result.height = std::min(height, layout.size.height - layout.filled_size.height);

  layout.filled_size.height += result.height;

  return result;
}

XXX_ALWAYS_INLINE int align(int inner_width, int parent_width, xxx::align alignment) noexcept {
  if (XXX_LIKELY(parent_width > inner_width)) {
    switch (alignment) {
      case xxx::align::center:
        return (parent_width - inner_width) / 2;
      case xxx::align::right:
        return (parent_width - inner_width);
      default:
        break;
    }
  }
  return 0;
}

constexpr std::size_t get_key_index(std::uint16_t key) noexcept {
  return key <= 0xFF ? key : (((0xFFFF - key) & 0xFF) + 256);
}

}  // namespace detail

void init() {
  // Init termbox.
  if (int rc = ::tb_init(); rc < 0) {
    std::string reason;
    switch (rc) {
      case TB_EUNSUPPORTED_TERMINAL: {
        reason = "unsuported terminal";
      } break;
      case TB_EFAILED_TO_OPEN_TTY: {
        reason = "failed to open tty";
      } break;
      case TB_EPIPE_TRAP_ERROR: {
        reason = "pipe trap error";
      } break;
      default: { reason = "unknown"; } break;
    }
    throw std::runtime_error{"Failed to init terminal library (" + reason + ")"};
  }

  // Use rbg colors.
  ::tb_select_output_mode(TB_OUTPUT_256);

  // Setup default style.
  ctx.style.panel.title_color = make_color(192, 41, 66) | attribute::bold;
  ctx.style.panel.border_color = make_color(84, 36, 55);
  ctx.style.panel.border = {L'│', L'─', L'╭', L'╮', L'╰', L'╯'};

  ctx.style.spinner.spinner_color = make_color(236, 208, 120) | attribute::bold;
  ctx.style.spinner.label_color = color::default_;
  ctx.style.spinner.glyphs = {{L'⠉', L'⠑', L'⠃', L'⠊', L'⠒', L'⠢', L'⠆', L'⠔', L'⠤', L'⢄', L'⡄', L'⡠',
                               L'⣀', L'⢄', L'⢠', L'⡠', L'⠤', L'⠢', L'⠰', L'⠔', L'⠒', L'⠑', L'⠘', L'⠊'}};

  ctx.style.progress.bar_color = make_color(14, 83, 180);
  ctx.style.progress.label_color = color::default_ | attribute::bold;
  ctx.style.progress.bar_glyph = L'│';

  ctx.style.text_input.fg = color::default_ | attribute::bold;
  ctx.style.text_input.bg = color::default_;

  // Preallocate container resources.
  ctx.layout_stack.reserve(8);
  ctx.input_queue_chars.reserve(16);

  // Update clock.
  ctx.timestamp = clock::now();
}

void shutdown() {
  // Shutdown termbox.
  ::tb_shutdown();
}

bool update(unsigned ms) {
  ctx.pressed_keys.fill(false);

  ::tb_event event;

  int result = ::tb_peek_event(&event, ms);
  switch (result) {
    case TB_EVENT_KEY: {
      if (event.ch > 0) {
        ctx.input_queue_chars.push_back(event.ch);
      }
      if (event.key > 0) {
        ctx.pressed_keys[detail::get_key_index(event.key)] = true;
      }
    } break;
    case TB_EVENT_RESIZE:
    case TB_EVENT_MOUSE:
      break;
    default: { } break; }

  auto now = clock::now();
  ctx.deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - ctx.timestamp).count() / 1000.0;
  ctx.timestamp = now;

  return result > 0;
}

bool is_key_pressed(key k) { return ctx.pressed_keys[detail::get_key_index(static_cast<std::uint16_t>(k))]; }

void begin() {
  ctx.screen_size = {::tb_width(), ::tb_height()};
  ctx.layout_stack.clear();

  // Prepare main layout.
  auto& layout = ctx.layout_stack.emplace_back();
  layout.type = layout_type::container;
  layout.size = ctx.screen_size;
  layout.pos = {0, 0};
  layout.filled_size = {0, 0};

  // Clear screen.
  ::tb_clear();
}

void end() {
  // Flush to terminal.
  ::tb_present();

  // Clear input chars queue.
  ctx.input_queue_chars.clear();
}

void row_begin(std::size_t columns) {
  if (columns == 0) {
    return;
  }

  // Parent layout for new row layout.
  auto& parent = ctx.layout_stack.back();

  // New layout.
  layout_state row;
  row.type = layout_type::row;
  row.size = {parent.size.width, parent.size.height - parent.filled_size.height};
  row.pos = {parent.pos.x, parent.pos.y + parent.filled_size.height};
  row.columns = columns;
  row.column = 0;
  row.filled_size = {0, 0};

  // Add layout to stack.
  ctx.layout_stack.push_back(row);
}

void row_push(float ratio_or_width) {
  if (XXX_UNLIKELY(ratio_or_width < 0.0)) {
    ratio_or_width = 0.0;
  }

  if (ctx.layout_stack.back().type == layout_type::row) {
    auto& row = ctx.layout_stack.back();

    assert(row.filled_size.width == 0);
    assert(row.column == 0);

    layout_state column;
    column.type = layout_type::column;
    column.pos = row.pos;

    // Calculate column width.
    int width = 0;
    if (ratio_or_width > 1.0) {
      width = std::min<int>(ratio_or_width, row.size.width);
    } else {
      width = std::round(ratio_or_width * row.size.width);
    }

    column.size = {width, row.size.height};

    row.filled_size.width = column.size.width;
    row.column += 1;

    // Add layout to stack.
    ctx.layout_stack.push_back(column);
  } else if (ctx.layout_stack.back().type == layout_type::column) {
    int filled_height = ctx.layout_stack.back().filled_size.height;
    // Remove prev column layout.
    ctx.layout_stack.pop_back();

    // Parent row layout.
    auto& row = ctx.layout_stack.back();

    if (XXX_UNLIKELY(row.type != layout_type::row)) {
      assert(false && "unexpected layout state");
      return;
    }

    if (XXX_UNLIKELY(row.column >= row.columns)) {
      assert(false && "unexpected layout state");
      return;
    }

    // Calculate new filled height.
    row.filled_size.height = std::max(row.filled_size.height, filled_height);

    layout_state column;
    column.type = layout_type::column;
    column.pos = {row.pos.x + row.filled_size.width, row.pos.y};

    // Calculate column width.
    int available_width = row.size.width - row.filled_size.width;
    int width = 0;
    if (ratio_or_width > 1.0) {
      width = std::min<int>(ratio_or_width, available_width);
    } else {
      width = std::min<int>(std::round(ratio_or_width * row.size.width), available_width);
    }

    column.size = {width, row.size.height};

    row.filled_size.width += column.size.width;
    row.column += 1;

    ctx.layout_stack.push_back(column);
  }
}

void row_end() {
  int row_filled_height = 0;

  if (ctx.layout_stack.back().type == layout_type::column) {
    row_filled_height = ctx.layout_stack.back().filled_size.height;
    ctx.layout_stack.pop_back();
  }

  if (XXX_UNLIKELY(ctx.layout_stack.back().type != layout_type::row)) {
    assert(false && "unexpected layout state");
    return;
  }

  row_filled_height = std::max(row_filled_height, ctx.layout_stack.back().filled_size.height);
  ctx.layout_stack.pop_back();

  // Update parent layout filled height.
  auto& parent = ctx.layout_stack.back();
  parent.filled_size.height += row_filled_height;
}

void panel_begin(std::string_view title) {
  auto& parent = ctx.layout_stack.back();

  if (XXX_UNLIKELY(parent.size.width < 2 || parent.size.height < 2)) {
    // TODO: may be this is wrong
    return;
  }

  // New layout for panel.
  layout_state panel;
  panel.type = layout_type::container;
  panel.size = {parent.size.width - 2, parent.size.height - parent.filled_size.height - 2};
  panel.pos = {parent.pos.x + 1, parent.pos.y + parent.filled_size.height + 1};

  int title_length = 0;
  if (title.size() > 0) {
    title_length = std::min<int>(utf8_string_length(title), panel.size.width - 2);
  }

  auto cell = make_cell(ctx.style.panel.border.horizontal_line, ctx.style.panel.border_color);
  draw_horizontal_line(panel.pos.x, panel.pos.y - 1, panel.size.width + 1, cell);
  cell.ch = ctx.style.panel.border.upper_left_corner;
  draw_cell(panel.pos.x - 1, panel.pos.y - 1, cell);
  cell.ch = ctx.style.panel.border.upper_right_corner;
  draw_cell(panel.pos.x - 1 + panel.size.width + 1, panel.pos.y - 1, cell);

  if (title_length > 0) {
    draw_text(panel.pos.x + 1, panel.pos.y - 1, title.data(), title_length, ctx.style.panel.title_color);
  }

  ctx.layout_stack.push_back(panel);
}

void panel_end() {
  if (XXX_UNLIKELY(ctx.layout_stack.back().type != layout_type::container)) {
    assert(false && "unexpected layout state");
    return;
  }

  int filled_height = ctx.layout_stack.back().filled_size.height;
  ctx.layout_stack.pop_back();

  auto& parent = ctx.layout_stack.back();
  parent.filled_size.height += filled_height + 2;

  auto cell = make_cell(ctx.style.panel.border.horizontal_line, ctx.style.panel.border_color);
  draw_horizontal_line(parent.pos.x + 1, parent.pos.y + parent.filled_size.height - 1, parent.size.width - 2, cell);
  cell.ch = ctx.style.panel.border.bottom_left_corner;
  draw_cell(parent.pos.x, parent.pos.y + parent.filled_size.height - 1, cell);
  cell.ch = ctx.style.panel.border.bottom_right_corner;
  draw_cell(parent.pos.x + parent.size.width - 1, parent.pos.y + parent.filled_size.height - 1, cell);
  cell.ch = ctx.style.panel.border.vertical_line;
  draw_vertical_line(parent.pos.x, parent.pos.y + parent.filled_size.height - (filled_height + 1), filled_height, cell);
  draw_vertical_line(parent.pos.x + parent.size.width - 1,
                     parent.pos.y + parent.filled_size.height - (filled_height + 1), filled_height, cell);
}

void spacer(float ratio_or_height) {
  if (XXX_UNLIKELY(ratio_or_height < 0.0)) {
    ratio_or_height = 0.0;
  }

  auto& parent = ctx.layout_stack.back();
  int available_height = parent.size.height - parent.filled_size.height;

  int height = 0;
  if (ratio_or_height > 1.0) {
    height = std::min<int>(ratio_or_height, available_height);
  } else {
    height = std::min<int>(std::round(ratio_or_height * available_height), available_height);
  }

  parent.filled_size.height += height;
}

void text(std::string_view str, color text_color, align alignment) {
  auto rect = detail::reserve_space(1);
  if (XXX_UNLIKELY(rect.width < 1 || rect.height < 1 || str.empty())) {
    return;
  }

  int str_length = std::min<int>(utf8_string_length(str), rect.width);
  int offset_x = detail::align(str_length, rect.width, alignment);
  draw_text(rect.x + offset_x, rect.y, str.data(), str_length, text_color);
}

void spinner(float& step_storage, std::string_view text, align alignment) {
  auto rect = detail::reserve_space(1);
  if (XXX_UNLIKELY(rect.width < 1 || rect.height < 1 || ctx.style.spinner.glyphs.empty())) {
    assert(!ctx.style.spinner.glyphs.empty() && "spinner frames not configured");
    return;
  }

  int str_length = 0;
  int inner_length = 1;
  if (text.size() > 0) {
    str_length = std::min<int>(utf8_string_length(text), rect.width - 2);
    inner_length += (str_length + 1);
  }

  int offset_x = detail::align(inner_length, rect.width, alignment);

  // Spinner
  static constexpr float spin_interval = 0.1;  // 0.1 seconds
  step_storage += ctx.deltaTime;
  std::size_t const index = std::size_t(std::round(step_storage / spin_interval)) % ctx.style.spinner.glyphs.size();

  // Spinner text
  draw_cell(rect.x + offset_x, rect.y, make_cell(ctx.style.spinner.glyphs[index], ctx.style.spinner.spinner_color));

  if (str_length > 0) {
    draw_text(rect.x + offset_x + 2, rect.y, text.data(), str_length, ctx.style.spinner.label_color);
  }
}

void progress(float& value) {
  auto rect = detail::reserve_space(1);
  if (XXX_UNLIKELY(rect.width < 1 || rect.height < 1)) {
    return;
  }

  // Bar
  value = std::clamp<float>(value, 0.0, 100.0);
  int length = std::round((rect.width * value) / 100.0);
  auto const cell = make_cell(ctx.style.progress.bar_glyph, ctx.style.progress.bar_color);
  draw_horizontal_line(rect.x, rect.y, length, cell);

  // Text
  char buffer[sizeof(" 100.0% ")];
  std::snprintf(buffer, sizeof(buffer), " %02.1f%% ", double(value));
  std::string_view text{buffer};

  int str_length = std::min<int>(text.size(), rect.width);
  int offset_x = detail::align(str_length, rect.width, align::center);
  draw_text(rect.x + offset_x, rect.y, text.data(), str_length, ctx.style.progress.label_color);
}

bool text_input(std::string& input) {
  u32_to_utf8(ctx.input_queue_chars.begin(), ctx.input_queue_chars.end(), std::back_inserter(input));

  if (ctx.pressed_keys[TB_KEY_SPACE]) {
    input.push_back(' ');
  }

  if (ctx.pressed_keys[TB_KEY_BACKSPACE2] && !input.empty()) {
    input.erase(utf8_prev(input.end()), input.end());
  }

  if (ctx.pressed_keys[TB_KEY_CTRL_W]) {
    while (!input.empty() && std::isblank(input.back())) {
      input.pop_back();
    }
    // Remove till first space from back or whole line.
    auto found = input.rfind(' ');
    if (found == input.npos) {
      input.clear();
    } else {
      input.erase(input.begin() + found + 1, input.end());
    }
  }

  bool result = ctx.pressed_keys[TB_KEY_ENTER] && input.size() > 0;

  auto rect = detail::reserve_space(1);
  if (XXX_UNLIKELY(rect.width < 1 || rect.height < 1)) {
    return result;
  }

  int input_length = utf8_string_length(input);
  int input_offset = input_length - (rect.width - 1);
  if (input_offset < 0) {
    input_offset = 0;
  }

  draw_text(rect.x, rect.y, input.data(), input_length, input_offset, ctx.style.text_input.fg, ctx.style.text_input.bg);
  draw_cell(rect.x + (input_length - input_offset), rect.y,
            make_cell(' ', ctx.style.text_input.bg, ctx.style.text_input.fg));

  return result;
}

}  // namespace xxx
