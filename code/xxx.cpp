
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

#include "hash.h"
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

template <typename OutputIt>
auto to_unicode(std::string_view input, OutputIt first) -> OutputIt {
  for_each_codepoint(input, [&](std::uint32_t ch) { *first++ = ch; });
  return first;
}

// decode into frame allocator: valid until next frame
[[nodiscard]] auto to_unicode(std::string_view input) noexcept -> std::span<std::uint32_t const> {
  // a codepoint takes at least one byte
  auto buffer = g_ctx->allocator.allocate<std::uint32_t>(input.size());
  if (!buffer) [[unlikely]] {
    return std::span<std::uint32_t const>();
  }
  auto pos = std::size_t(0);
  for_each_codepoint(input, [&](std::uint32_t ch) { buffer[pos++] = ch; });
  return std::span(buffer, pos);
}

template <typename OutputIt>
auto to_utf8(std::span<std::uint32_t const> input, OutputIt first) -> OutputIt {
  char buffer[4];
  for (auto const ch : input) {
    first = std::copy_n(buffer, utf8_encode(ch, buffer), first);
  }
  return first;
}

// std::isblank is UB for values outside unsigned char range, so check codepoints explicitly
[[nodiscard]] constexpr auto is_blank_codepoint(std::uint32_t ch) noexcept -> bool {
  return ch == U' ' || ch == U'\t';
}

[[nodiscard]] auto get_style_bg(im_color_id bg_id) noexcept -> im_style {
  return im_style(im_color(), g_ctx->theme.get_color(bg_id));
}

[[nodiscard]] auto get_style(im_color_id fg_id, im_color_id bg_id) noexcept -> im_style {
  return g_ctx->theme.get_style(fg_id, bg_id);
}

} // namespace

void init(std::unique_ptr<im_backend> backend) {
  assert(backend);

  // previous backend must be released before new one starts using terminal
  shutdown();

  auto ctx = std::make_unique<im_context>();
  ctx->allocator.reserve(2 * 1024 * 1024);
  ctx->backend = std::move(backend);
  ctx->last_frame_time = ctx->backend->now();
  g_ctx = ctx.release();
}

void init() {
  // shutdown first: termbox can't be initialized twice
  shutdown();
  init(make_termbox_backend());
}

void shutdown() {
  delete g_ctx;
  g_ctx = nullptr;
}

void process_input_events() {
  process_input_events(std::chrono::milliseconds(0));
}

auto process_input_events(std::chrono::milliseconds timeout) -> bool {
  assert(g_ctx);

  g_ctx->input.reset();
  auto const got_event = g_ctx->backend->poll_events(g_ctx->input, timeout);

  auto& view = g_ctx->view;
  auto& widget = g_ctx->widget;
  if (is_key_pressed(im_key_id::tab)) {
    if (widget.next_id != im_id()) {
      widget.active_id = widget.next_id;
    } else if (widget.first_id != im_id()) {
      widget.active_id = widget.first_id;
    }
  } else if (is_key_pressed(im_key_id::back_tab)) {
    if (widget.prev_id != im_id()) {
      widget.active_id = widget.prev_id;
    } else if (widget.last_id != im_id()) {
      widget.active_id = widget.last_id;
    }
  }

  if (view.force_next_id != im_id()) {
    if (view.active_id != view.force_next_id) {
      view.active_id = view.force_next_id;
      // reset active widget_id on active_id changed
      widget.active_id = im_id();
    }
  }

  // focus requested on previous frame
  if (auto const target = std::exchange(widget.focus_target, {}); target.id != im_id()) {
    if (g_ctx->popup.open_id == im_id()) {
      view.active_id = target.view_id;
      widget.active_id = target.id;
    } else if (target.in_popup) {
      // modal: only popup widgets can be focused
      widget.active_id = target.id;
    }
  }

  // mouse click: activate view and focus widget under cursor
  widget.clicked_id = im_id();
  if (g_ctx->input.is_mouse_clicked(im_mouse_button_id::left)) {
    auto const pos = g_ctx->input.mouse_clicked_pos(im_mouse_button_id::left);
    auto const hit = [&](auto const& item) { return item.rect.contains(pos); };

    // later drawn is on top; open popup takes all clicks
    auto const popup_id = g_ctx->popup.open_id;
    auto const& views = g_ctx->view_hits;
    auto const view_hit = [&](auto const& item) { return (popup_id == im_id() || item.id == popup_id) && hit(item); };
    if (auto const v = std::find_if(views.rbegin(), views.rend(), view_hit); v != views.rend()) {
      if (view.active_id != v->id) {
        view.active_id = v->id;
        widget.active_id = im_id();
      }
      auto const& widgets = g_ctx->widget_hits;
      auto const w = std::find_if(widgets.rbegin(), widgets.rend(),
          [&](auto const& item) { return item.view_id == v->id && hit(item); });
      if (w != widgets.rend()) {
        widget.active_id = w->id;
        widget.clicked_id = w->id;
        widget.clicked_pos = pos;
      }
    }
  }
  return got_event;
}

void new_frame() {
  assert(g_ctx);

  g_ctx->allocator.reset();
  // hit areas of previous frame were consumed by process_input_events()
  g_ctx->view_hits.clear();
  g_ctx->widget_hits.clear();
  g_ctx->frame_ids.clear();

  // TODO: frame delta

  auto const screen_rect = get_screen_rect();

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
  g_ctx->widget.prev_id = im_id();
  g_ctx->widget.last_id = im_id();
  g_ctx->widget.focus_next = false;
  if (g_ctx->widget.focus_request_id != im_id() && ++g_ctx->widget.focus_request_age > 1) {
    // widget wasn't built during request frame and the next one
    g_ctx->widget.focus_request_id = im_id();
  }
  g_ctx->next_item_width = 0;
  g_ctx->widget.active = false;
  g_ctx->widget.pressed = false;

  auto const now = g_ctx->backend->now();
  g_ctx->elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_ctx->last_frame_time).count() * 0.001f;
  g_ctx->last_frame_time = now;

  g_ctx->renderer.set_clear_color(g_ctx->theme.get_style(im_color_id::text, im_color_id::background));
  g_ctx->renderer.start_new_frame(screen_rect);
}

namespace {

// count ids used more than once this frame, mark them on top of everything
void check_id_collisions() {
  auto& sorted = g_ctx->frame_ids_sorted;
  sorted.assign(g_ctx->frame_ids.begin(), g_ctx->frame_ids.end());
  std::stable_sort(sorted.begin(), sorted.end(), [](auto const& a, auto const& b) { return a.id < b.id; });

  auto& renderer = g_ctx->renderer;
  auto const saved_layer = renderer.layer();
  if (g_ctx->show_id_collisions) {
    renderer.set_layer(2);
    renderer.push_clip_rect(get_screen_rect(), false);
  }

  static constexpr auto marker = std::uint32_t('!');
  auto const style = im_style(0xffffff_c, 0xd70000_c);
  g_ctx->id_collisions = 0;
  for (auto first = sorted.begin(); first != sorted.end();) {
    auto const last = std::find_if(first, sorted.end(), [&](auto const& item) { return item.id != first->id; });
    if (std::distance(first, last) > 1) {
      ++g_ctx->id_collisions;
      if (g_ctx->show_id_collisions) {
        for (auto it = first; it != last; ++it) {
          if (it->visible) {
            renderer.cmd_draw_text_at(it->visible.min, std::span<std::uint32_t const>(&marker, 1), style);
          }
        }
      }
    }
    first = last;
  }

  if (g_ctx->show_id_collisions) {
    renderer.pop_clip_rect();
    renderer.set_layer(saved_layer);
  }
}

} // namespace

void render() {
  assert(g_ctx);

  check_id_collisions();
  g_ctx->renderer.render(*g_ctx->backend);
}

void set_clipboard(std::string_view text) {
  g_ctx->backend->set_clipboard(text);
}

auto id_collision_count() -> int {
  return g_ctx->id_collisions;
}

void show_id_collisions(bool show) {
  g_ctx->show_id_collisions = show;
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
  assert(g_ctx);
  auto const size = g_ctx->backend->size();
  return im_rect(0, 0, size.x - 1, size.y - 1);
}

auto is_key_pressed(im_key_id id) -> bool {
  return g_ctx->input.is_key_pressed(id);
}

auto key_press_count(im_key_id id) -> int {
  return g_ctx->input.key_press_count(id);
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

  auto const& parent_layout = g_ctx->layout.layout_state_stack.back();
  auto& row_layout = g_ctx->layout.layout_state_stack.emplace_back();

  // reset cursor to start of container layout
  g_ctx->layout.cursor.x = parent_layout.rect.min.x;

  row_layout.type = im_layout_type::row;
  row_layout.rect.min = g_ctx->layout.cursor;
  row_layout.rect.max = im_vec2(parent_layout.rect.max.x, g_ctx->layout.cursor.y);
  row_layout.row = im_layout_data_row{.columns = int(columns), .index = 0, .cursor_max_y = g_ctx->layout.cursor.y};

  // at this point
  // row_layout.rect.max.x != row_layout.rect.min.x -> layout width set
  // row_layout.rect.max.y == row_layout.rect.min.y -> layout height unset (dynamic)
}

void layout_row_push(float ratio_or_width) {

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

  auto width = 0;
  if (ratio_or_width < 0.0f) {
    // fill(n): rest of the row minus n columns
    auto const leave = int(-ratio_or_width) - 1;
    width = std::max(0, row_layout.rect.max.x - offset_min_x + 1 - leave);
  } else if (ratio_or_width > 1.0f) {
    width = int(ratio_or_width);
  } else {
    width = int(std::ceil(ratio_or_width * row_layout.rect.width()));
  }
  auto const offset_max_x = std::min<int>(offset_min_x + width - 1, row_layout.rect.max.x);

  auto& column_layout = g_ctx->layout.layout_state_stack.emplace_back();
  column_layout.type = im_layout_type::column;
  column_layout.rect.min = im_vec2(offset_min_x, row_layout.rect.min.y);
  column_layout.rect.max = im_vec2(offset_max_x, row_layout.rect.min.y);
  column_layout.none = {};

  row_layout.row.index++;

  g_ctx->layout.cursor = column_layout.rect.min;
  g_ctx->layout.last_cursor_y = g_ctx->layout.cursor.y;
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

  auto const& parent_layout = g_ctx->layout.layout_state_stack.back();

  g_ctx->layout.cursor = im_vec2(parent_layout.rect.min.x, cursor_max_y);
  g_ctx->layout.last_cursor_y = g_ctx->layout.cursor.y;
}

void same_line() {
  g_ctx->layout.same_line = true;
}

void set_next_item_width(int width) {
  g_ctx->next_item_width = width;
}

void set_focus(std::string_view label) {
  auto const [str, key] = g_ctx->hash_id.split_str_key(label);
  g_ctx->widget.focus_request_id = g_ctx->hash_id.make(key);
  g_ctx->widget.focus_request_age = 0;
}

void set_focus_next() {
  g_ctx->widget.focus_next = true;
}

auto is_item_focused() -> bool {
  return g_ctx->widget.active;
}

void push_id(std::string_view id) {
  g_ctx->hash_id.push_id(id);
}

void push_id(int id) {
  g_ctx->hash_id.push_id(id);
}

void pop_id() {
  g_ctx->hash_id.pop_id();
}

namespace internal {

// width for widget being added: set_next_item_width(...) or default, fill(n) resolved against current line
[[nodiscard]] auto item_width(int default_width) noexcept -> int {
  auto width = std::exchange(g_ctx->next_item_width, 0);
  if (width == 0) {
    width = default_width;
  }
  if (width > 0) {
    return width;
  }
  auto const& layout = g_ctx->layout.layout_state_stack.back();
  auto const x = g_ctx->layout.same_line ? g_ctx->layout.cursor.x : layout.rect.min.x;
  return std::max(1, layout.rect.max.x - x + 1 - (-width - 1));
}

} // namespace internal

void view_begin(std::string_view name, int flags, im_key_id shortcut, int height) {
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
  auto const popup_open = g_ctx->popup.open_id != im_id();
  view.active = (view.active_id == view.current_id) && !popup_open;

  if (is_key_pressed(shortcut) && !view.active && !popup_open) {
    view.force_next_id = view.current_id;
  }
  // TODO: skip frame?

  // layout and visuals
  {
    auto const& parent_layout = g_ctx->layout.layout_state_stack.back();
    auto const available_bottom = g_ctx->layout.available_bottom();
    auto& layout = g_ctx->layout.layout_state_stack.emplace_back();

    auto const do_render_border = (im_view_flag_border == (flags & im_view_flag_border));
    auto const do_render_title = (im_view_flag_title == (flags & im_view_flag_title));
    auto const border = do_render_border ? 1 : 0;

    // reset cursor to start of container layout
    g_ctx->layout.cursor.x = parent_layout.rect.min.x;

    auto const top = g_ctx->layout.cursor.y;
    view.current_bounded = (height != 0);
    if (height > 0) {
      view.current_bottom = top + height - 1;
    } else if (height < 0) {
      // fill(n) == -1 - n
      view.current_bottom = std::max(top, available_bottom - (-height - 1));
    }

    layout.type = im_layout_type::container;
    layout.rect.min = g_ctx->layout.cursor + im_vec2(border, border);
    layout.rect.max = im_vec2(parent_layout.rect.max.x - border, g_ctx->layout.cursor.y);
    layout.container = im_layout_data_container{.border = border, .fixed_height = view.current_bounded};
    if (view.current_bounded) {
      layout.rect.max.y = view.current_bottom - border;
    }

    g_ctx->layout.cursor = layout.rect.min;
    g_ctx->layout.last_cursor_y = g_ctx->layout.cursor.y;

    if (!do_render_border && do_render_title) {
      auto const title_rect = g_ctx->layout.reserve_layout_lines(1);
      auto const style = view.active
                             ? g_ctx->theme.get_style(im_color_id::view_active_title, im_color_id::background)
                             : g_ctx->theme.get_style(im_color_id::view_inactive_title, im_color_id::background);
      g_ctx->renderer.cmd_fill_rect(title_rect, ' ', style);
      g_ctx->renderer.cmd_draw_text_in_rect(
          title_rect, to_unicode(view.current_title), style, im_halign::center, im_valign::top);
    }

    // shrink available clip rect by layout width
    auto clip_rect = g_ctx->renderer.clip_rect();
    clip_rect.min.x = layout.rect.min.x;
    clip_rect.max.x = layout.rect.max.x;

    view.current_scroll = nullptr;
    if (view.current_bounded) {
      auto& scroll = view.scroll[view.current_id];
      view.current_scroll = &scroll;
      view.current_viewport = im_rect(layout.rect.min.x, g_ctx->layout.cursor.y, layout.rect.max.x, layout.rect.max.y);

      auto const viewport_height = std::max(0, view.current_viewport.height());
      if (view.active) {
        auto const page = std::max(1, viewport_height - 1);
        scroll.offset += page * (key_press_count(im_key_id::page_down) - key_press_count(im_key_id::page_up));
      }
      if (auto const wheel = g_ctx->input.mouse_wheel(); wheel != 0 && !popup_open) {
        auto const mouse = g_ctx->input.mouse_pos();
        auto const view_rect = im_rect(layout.rect.min.x - border, top, layout.rect.max.x + border, view.current_bottom);
        if (view_rect.contains(mouse)) {
          constexpr auto wheel_step = int(3);
          scroll.offset += wheel * wheel_step;
        }
      }
      // content height is known from previous frame only
      scroll.offset = std::clamp(scroll.offset, 0, std::max(0, scroll.content_height - viewport_height));

      g_ctx->layout.cursor.y -= scroll.offset;
      g_ctx->layout.last_cursor_y = g_ctx->layout.cursor.y;
      view.current_content_top = g_ctx->layout.cursor.y;

      clip_rect.min.y = std::max(clip_rect.min.y, view.current_viewport.min.y);
      clip_rect.max.y = std::min(clip_rect.max.y, view.current_viewport.max.y);
    }
    g_ctx->renderer.push_clip_rect(clip_rect);
  }
}

void view_end() {
  auto& view = g_ctx->view;

  g_ctx->hash_id.pop_id();

  // layout and visuals
  {
    g_ctx->renderer.pop_clip_rect();

    auto const do_render_border = (im_view_flag_border == (view.current_flags & im_view_flag_border));
    auto const do_render_title = (im_view_flag_title == (view.current_flags & im_view_flag_title));

    auto& layout = g_ctx->layout.layout_state_stack.back();
    if (layout.type != im_layout_type::container) [[unlikely]] {
      assert(false && "view_end(...) out of order");
      return;
    }
    auto const border = layout.container.border;

    // calculate panel whole size
    auto const panel_bottom = view.current_bounded ? view.current_bottom : g_ctx->layout.cursor.y + border - 1;
    auto const panel_rect =
        im_rect(layout.rect.min - im_vec2(border, border), im_vec2(layout.rect.max.x + border, panel_bottom));

    g_ctx->layout.layout_state_stack.pop_back();

    auto const visible_panel = panel_rect.intersection(g_ctx->renderer.clip_rect());
    g_ctx->frame_ids.push_back({.id = view.current_id, .visible = visible_panel});
    if (visible_panel) {
      g_ctx->view_hits.push_back({.id = view.current_id, .view_id = view.current_id, .rect = visible_panel});
    }

    auto const border_style = view.active
                                  ? g_ctx->theme.get_style(im_color_id::view_active_border, im_color_id::background)
                                  : g_ctx->theme.get_style(im_color_id::view_inactive_border, im_color_id::background);

    if (do_render_border) {
      g_ctx->renderer.cmd_draw_rect(panel_rect, border_style);

      if (do_render_title) {
        auto const style = view.active
                               ? g_ctx->theme.get_style(im_color_id::view_active_title, im_color_id::background)
                               : g_ctx->theme.get_style(im_color_id::view_inactive_title, im_color_id::background);
        g_ctx->renderer.cmd_draw_text_in_rect(
            panel_rect, to_unicode(view.current_title), style, im_halign::center, im_valign::top);
      }
    }

    if (view.current_scroll) {
      auto& scroll = *view.current_scroll;
      auto const viewport_height = std::max(0, view.current_viewport.height());
      scroll.content_height = g_ctx->layout.cursor.y - view.current_content_top;
      // keep offset valid for next frame if content shrank
      scroll.offset = std::clamp(scroll.offset, 0, std::max(0, scroll.content_height - viewport_height));

      if (do_render_border && scroll.content_height > viewport_height && viewport_height > 0) {
        static constexpr auto thumb_ch = std::uint32_t(L'┃');
        auto const thumb_height =
            std::max(1, viewport_height * viewport_height / scroll.content_height);
        auto const max_offset = scroll.content_height - viewport_height;
        auto const thumb_pos = (viewport_height - thumb_height) * scroll.offset / max_offset;
        auto const x = panel_rect.max.x;
        auto const y = view.current_viewport.min.y + thumb_pos;
        g_ctx->renderer.cmd_fill_rect(im_rect(x, y, x, y + thumb_height - 1), thumb_ch, border_style);
      }
    }

    auto const& parent_layout = g_ctx->layout.layout_state_stack.back();
    g_ctx->layout.cursor = im_vec2(parent_layout.rect.min.x, panel_bottom + 1);
    g_ctx->layout.last_cursor_y = g_ctx->layout.cursor.y;
  }

  view.current_title = "N/A";
  view.current_id = im_id();
  view.current_flags = 0;
  view.active = false;
  view.current_bounded = false;
  view.current_scroll = nullptr;
}

namespace {

[[nodiscard]] auto popup_id(std::string_view id) noexcept -> im_id {
  // global (not scoped): open_popup and popup_begin may be called from different scopes
  return im_id(hash(id, 0x706f7075u));
}

void close_popup_now() noexcept {
  auto& popup = g_ctx->popup;
  popup.open_id = im_id();
  popup.close_requested = false;
  g_ctx->widget.active_id = popup.saved_widget_id;
}

} // namespace

void open_popup(std::string_view id) {
  auto& popup = g_ctx->popup;
  auto const pid = popup_id(id);
  if (popup.open_id == pid) {
    return;
  }
  if (popup.open_id == im_id()) {
    popup.saved_widget_id = g_ctx->widget.active_id;
  }
  popup.open_id = pid;
  popup.close_requested = false;
  // first popup widget takes focus
  g_ctx->widget.active_id = im_id();
  // the key or click that opened the popup is used up: it must not press a popup button
  g_ctx->input.consume_keyboard();
  g_ctx->widget.clicked_id = im_id();
}

void close_popup() {
  if (g_ctx->popup.current_id != im_id()) {
    // inside popup_begin / popup_end: finish building first
    g_ctx->popup.close_requested = true;
  } else {
    close_popup_now();
  }
}

auto is_popup_open() -> bool {
  return g_ctx->popup.open_id != im_id();
}

auto popup_begin(std::string_view id, std::string_view title, int width) -> bool {
  auto& popup = g_ctx->popup;
  auto& view = g_ctx->view;
  auto const pid = popup_id(id);
  if (popup.open_id != pid) {
    return false;
  }
  if (view.current_id != im_id() || popup.current_id != im_id()) [[unlikely]] {
    assert(false && "popup_begin(...) inside view or popup");
    return false;
  }
  if (is_key_pressed(im_key_id::esc)) {
    close_popup_now();
    return false;
  }

  auto const screen = get_screen_rect();
  auto const it = popup.heights.find(pid);
  popup.measuring = (it == popup.heights.end());
  auto const w = std::clamp(width, 4, std::max(4, screen.width()));
  auto const h = (popup.measuring ? 1 : it->second) + 2;
  auto const left = screen.min.x + std::max(0, (screen.width() - w) / 2);
  auto const top = screen.min.y + std::max(0, (screen.height() - h) / 2);

  popup.current_id = pid;
  popup.current_title = std::format(" {} ", title);
  popup.current_rect = im_rect(left, top, left + w - 1, top + h - 1);

  // own layout, surrounding layout is restored in popup_end
  popup.saved_cursor = g_ctx->layout.cursor;
  popup.saved_last_cursor_y = g_ctx->layout.last_cursor_y;
  popup.saved_same_line = g_ctx->layout.same_line;
  auto& layout = g_ctx->layout.layout_state_stack.emplace_back();
  layout.type = im_layout_type::container;
  layout.rect = im_rect(left + 1, top + 1, left + w - 2, top + 1);
  layout.container = im_layout_data_container{.border = 1, .fixed_height = false};
  g_ctx->layout.cursor = layout.rect.min;
  g_ctx->layout.last_cursor_y = layout.rect.min.y;
  g_ctx->layout.same_line = false;

  // popup acts as the only active view
  view.current_id = pid;
  view.current_flags = 0;
  view.active = true;
  view.current_bounded = false;
  view.current_scroll = nullptr;
  g_ctx->hash_id.push_id(id);

  popup.saved_layer = g_ctx->renderer.layer();
  g_ctx->renderer.set_layer(2);
  g_ctx->renderer.push_clip_rect(im_rect(left + 1, top + 1, left + w - 2, screen.max.y).intersection(screen), false);
  return true;
}

void popup_end() {
  auto& popup = g_ctx->popup;
  auto& view = g_ctx->view;
  if (popup.current_id == im_id()) [[unlikely]] {
    assert(false && "popup_end(...) without popup_begin(...)");
    return;
  }

  g_ctx->renderer.pop_clip_rect();
  g_ctx->hash_id.pop_id();

  auto& rect = popup.current_rect;
  auto const content_height = g_ctx->layout.cursor.y - (rect.min.y + 1);
  popup.heights[popup.current_id] = content_height;
  rect.max.y = rect.min.y + content_height + 1;

  // background and frame below content
  auto const screen = get_screen_rect();
  g_ctx->renderer.set_layer(1);
  g_ctx->renderer.push_clip_rect(screen, false);
  g_ctx->renderer.cmd_fill_rect(rect, ' ', get_style_bg(im_color_id::background));
  g_ctx->renderer.cmd_draw_rect(rect, get_style(im_color_id::view_active_border, im_color_id::background));
  g_ctx->renderer.cmd_draw_text_in_rect(rect, to_unicode(popup.current_title),
      get_style(im_color_id::view_active_title, im_color_id::background), im_halign::center, im_valign::top);
  g_ctx->renderer.pop_clip_rect();
  g_ctx->renderer.set_layer(popup.saved_layer);

  if (popup.measuring) {
    // height was unknown: don't show misplaced popup, next frame is centered
    g_ctx->renderer.clear_layer(1);
    g_ctx->renderer.clear_layer(2);
  } else if (auto const visible = rect.intersection(screen); visible) {
    g_ctx->view_hits.push_back({.id = popup.current_id, .view_id = popup.current_id, .rect = visible});
  }

  g_ctx->layout.layout_state_stack.pop_back();
  g_ctx->layout.cursor = popup.saved_cursor;
  g_ctx->layout.last_cursor_y = popup.saved_last_cursor_y;
  g_ctx->layout.same_line = popup.saved_same_line;

  view.current_id = im_id();
  view.active = false;
  popup.current_id = im_id();

  if (popup.close_requested) {
    close_popup_now();
  }
}

void panel_begin() {
  constexpr auto border = int(1);

  auto const& parent_layout = g_ctx->layout.layout_state_stack.back();
  auto& layout = g_ctx->layout.layout_state_stack.emplace_back();

  // reset cursor to start of container layout
  g_ctx->layout.cursor.x = parent_layout.rect.min.x;

  layout.type = im_layout_type::container;
  layout.rect.min = g_ctx->layout.cursor + im_vec2(border, border);
  layout.rect.max = im_vec2(parent_layout.rect.max.x - border, g_ctx->layout.cursor.y);
  layout.container = im_layout_data_container{.border = border};

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

  // restore cursor position at x
  auto const& parent_layout = g_ctx->layout.layout_state_stack.back();
  g_ctx->layout.cursor = im_vec2(parent_layout.rect.min.x, g_ctx->layout.cursor.y + border);
}

void label(std::string_view text) {
  auto const unicode_text = to_unicode(text);
  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(text_width(unicode_text), 1));
  if (!g_ctx->renderer.is_visible(widget_rect)) {
    return;
  }
  auto const style = g_ctx->theme.get_style(im_color_id::text, im_color_id::background);
  g_ctx->renderer.cmd_fill_rect(widget_rect, ' ', style);
  g_ctx->renderer.cmd_draw_text_in_rect(widget_rect, unicode_text, style, im_halign::left, im_valign::top);
}

namespace internal {

// update widget.* properties
void common_focusable_behaviour(im_id widget_id, im_rect const& widget_rect) noexcept {
  auto& view = g_ctx->view;
  auto& widget = g_ctx->widget;

  widget.current_id = widget_id;
  widget.pressed = false;
  widget.active = false;

  if (view.current_id != im_id() &&
      (std::exchange(widget.focus_next, false) || widget.focus_request_id == widget_id)) {
    widget.focus_request_id = im_id();
    widget.focus_target = {.id = widget_id, .view_id = view.current_id, .in_popup = g_ctx->popup.current_id != im_id()};
  }

  // TODO:
  // https://github.com/ksergey/xxx/blob/20b7cfdab337acda6e69290a8f4a9ee408ad8c37/code_v3/xxx.cpp#L306-L316

  // focus staff: while popup is open only its widgets take part, even in a view
  // started before the popup was opened in this frame
  auto const& popup = g_ctx->popup;
  auto const focusable = view.active && (popup.open_id == im_id() || popup.current_id == popup.open_id);
  if (focusable) {
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
      // previous focusable in this view; none if active is first (wraps to last_id)
      widget.prev_id = widget.last_id;
    }
    widget.last_id = widget.current_id;
  }

  auto const visible = widget_rect.intersection(g_ctx->renderer.clip_rect());
  g_ctx->frame_ids.push_back({.id = widget_id, .visible = visible});
  if (view.current_id != im_id() && visible) {
    g_ctx->widget_hits.push_back({.id = widget_id, .view_id = view.current_id, .rect = visible});
  }

  // scroll focused widget into view once, when focus arrives (applied next frame)
  if (widget.active && view.current_scroll && view.current_scroll->focus_id != widget.current_id) {
    auto& scroll = *view.current_scroll;
    scroll.focus_id = widget.current_id;
    auto const& viewport = view.current_viewport;
    if (widget_rect.min.y < viewport.min.y) {
      scroll.offset -= viewport.min.y - widget_rect.min.y;
    } else if (widget_rect.max.y > viewport.max.y) {
      scroll.offset += widget_rect.max.y - viewport.max.y;
    }
  }
}

} // namespace internal

auto button(std::string_view label) -> bool {
  static constexpr int button_min_width = 10;
  static constexpr auto fx_left_ch = std::uint32_t(L'[');
  static constexpr auto fx_right_ch = std::uint32_t(L']');

  auto& widget = g_ctx->widget;

  auto const [str, widget_key] = g_ctx->hash_id.split_str_key(label);
  auto const unicode_str = to_unicode(str);
  auto const unicode_str_width = text_width(unicode_str);
  auto const button_width = std::max<int>(button_min_width, unicode_str_width + 4);
  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(button_width, 1));

  internal::common_focusable_behaviour(g_ctx->hash_id.make(widget_key), widget_rect);

  if (widget.active) {
    if (is_key_pressed(im_key_id::space) || is_key_pressed(im_key_id::enter) ||
        widget.clicked_id == widget.current_id) {
      widget.pressed = true;
    }
  }

  if (g_ctx->renderer.is_visible(widget_rect)) {
    // fill background
    g_ctx->renderer.cmd_fill_rect(widget_rect, ' ',
        widget.active ? get_style_bg(im_color_id::button_active_background)
                      : get_style_bg(im_color_id::button_inactive_background));

    // label start pos (signed arithmetic: label may be wider than widget rect)
    auto const unicode_str_pos =
        widget_rect.min + im_vec2(std::max(0, (widget_rect.width() - unicode_str_width) / 2), 0);

    // draw label
    g_ctx->renderer.cmd_draw_text_at(unicode_str_pos, unicode_str,
        widget.active ? get_style(im_color_id::button_active_text, im_color_id::button_active_background)
                      : get_style(im_color_id::button_inactive_text, im_color_id::button_inactive_background));

    // draw left fx
    g_ctx->renderer.cmd_draw_text_at(unicode_str_pos - im_vec2(2, 0), std::span<std::uint32_t const>(&fx_left_ch, 1),
        widget.active ? get_style(im_color_id::button_active_fx, im_color_id::button_active_background)
                      : get_style(im_color_id::button_inactive_fx, im_color_id::button_inactive_background));

    // draw right fx
    g_ctx->renderer.cmd_draw_text_at(unicode_str_pos + im_vec2(unicode_str_width + 1, 0),
        std::span<std::uint32_t const>(&fx_right_ch, 1),
        widget.active ? get_style(im_color_id::button_active_fx, im_color_id::button_active_background)
                      : get_style(im_color_id::button_inactive_fx, im_color_id::button_inactive_background));
  }

  return widget.pressed;
}

auto text_input(std::string_view placeholder, std::string& input, int flags) -> bool {
  static constexpr int input_width = 16;
  static constexpr auto password_ch = std::uint32_t('*');
  auto const password = (flags & im_input_flag_password) != 0;
  // cells taken by char as displayed
  auto const display_width = [password](std::uint32_t ch) { return password ? 1 : char_width(ch); };
  // readline kill: erase [first, last), keep it for ctrl-y and system clipboard.
  // Password text goes nowhere: neither clipboard nor kill buffer (could be yanked into a plain field).
  auto const kill = [&](int first, int last) {
    auto& t = g_ctx->text_input;
    if (!password) {
      t.kill_buffer.assign(t.text.begin() + first, t.text.begin() + last);
      set_clipboard(unicode_to_utf8(t.kill_buffer));
    }
    t.text.erase(t.text.begin() + first, t.text.begin() + last);
  };
  static constexpr int input_prompt_width = 2; // "> "

  auto& widget = g_ctx->widget;
  auto& text_input = g_ctx->text_input;

  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(internal::item_width(input_width), 1));
  auto const [str, widget_key] = g_ctx->hash_id.split_str_key(placeholder);

  internal::common_focusable_behaviour(g_ctx->hash_id.make(widget_key), widget_rect);

  if (widget.active) {
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
    if (widget.clicked_id == widget.current_id) {
      // place cursor before the char under mouse (text is shown after prompt, shifted by scroll)
      auto const column = widget.clicked_pos.x - (widget_rect.min.x + input_prompt_width) + text_input.scroll_offset;
      auto cursor = 0;
      for (auto x = 0; cursor < int(text_input.text.size()); ++cursor) {
        x += display_width(text_input.text[cursor]);
        if (column < x) {
          break;
        }
      }
      text_input.cursor_pos = column < 0 ? 0 : cursor;
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
        case im_key_id::home:
        case im_key_id::ctrl_a: {
          text_input.cursor_pos = 0;
          text_input.scroll_offset = 0;
        } break;
        case im_key_id::end:
        case im_key_id::ctrl_e: {
          text_input.cursor_pos = text_input.text.size();
        } break;
        case im_key_id::ctrl_u: {
          // readline: kill to start of line
          if (text_input.cursor_pos > 0) {
            kill(0, text_input.cursor_pos);
            text_input.cursor_pos = 0;
            text_changed = true;
          }
        } break;
        case im_key_id::ctrl_k: {
          // readline: kill to end of line
          if (text_input.cursor_pos < int(text_input.text.size())) {
            kill(text_input.cursor_pos, int(text_input.text.size()));
            text_changed = true;
          }
        } break;
        case im_key_id::ctrl_y: {
          // readline: yank last killed text
          if (!text_input.kill_buffer.empty()) {
            text_input.text.insert(text_input.text.begin() + text_input.cursor_pos, text_input.kill_buffer.begin(),
                text_input.kill_buffer.end());
            text_input.cursor_pos += int(text_input.kill_buffer.size());
            text_changed = true;
          }
        } break;
        case im_key_id::ctrl_c: {
          if (!password && !text_input.text.empty()) {
            set_clipboard(unicode_to_utf8(text_input.text));
          }
        } break;
        case im_key_id::ctrl_w: {
          if (!text_input.text.empty()) {
            // deleted range is contiguous: remember old text to put it into kill buffer
            auto const before = text_input.text;
            if (auto const text_length = int(text_input.text.size()); text_input.cursor_pos >= text_length) {
              // on end of input move cursor to last char
              text_input.cursor_pos = text_length - 1;
            } else if (!is_blank_codepoint(text_input.text[text_input.cursor_pos])) {
              // keep symbol under cursor if non blank
              text_input.cursor_pos--;
            }
            // drop blanks before cursor
            while (text_input.cursor_pos >= 0 && is_blank_codepoint(text_input.text[text_input.cursor_pos])) {
              text_input.text.erase(text_input.text.begin() + text_input.cursor_pos--);
            }
            // drop until blank
            while (text_input.cursor_pos >= 0 && !is_blank_codepoint(text_input.text[text_input.cursor_pos])) {
              text_input.text.erase(text_input.text.begin() + text_input.cursor_pos--);
            }
            if (text_input.cursor_pos < 0) {
              text_input.cursor_pos = 0;
            } else {
              text_input.cursor_pos += 1;
            }
            if (auto const removed = before.size() - text_input.text.size(); removed > 0 && !password) {
              auto const first = before.begin() + text_input.cursor_pos;
              text_input.kill_buffer.assign(first, first + std::ptrdiff_t(removed));
              set_clipboard(unicode_to_utf8(text_input.kill_buffer));
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

    rect.min += im_vec2(text_width(prompt), 0);

    // "static" keywoard is required here
    static constexpr auto space_ch = std::uint32_t(' ');
    auto const space = std::span<std::uint32_t const>(&space_ch, 1);

    // text is drawn shifted inside own clip rect; renderer cuts it by columns
    g_ctx->renderer.push_clip_rect(rect);

    if (input.empty()) {
      auto const unicode_str = to_unicode(str);

      if (widget.active) {
        auto const cursor_style =
            get_style(im_color_id::input_active_text, im_color_id::input_active_background).with_reverse();
        if (!unicode_str.empty()) {
          g_ctx->renderer.cmd_draw_text_at(rect.min, substr(unicode_str, 0, 1), cursor_style);
          g_ctx->renderer.cmd_draw_text_at(rect.min + im_vec2(char_width(unicode_str[0]), 0), substr(unicode_str, 1),
              get_style(im_color_id::input_placeholder, im_color_id::input_active_background));
        } else {
          g_ctx->renderer.cmd_draw_text_at(rect.min, space, cursor_style);
        }
      } else if (!unicode_str.empty()) {
        g_ctx->renderer.cmd_draw_text_at(
            rect.min, unicode_str, get_style(im_color_id::input_placeholder, im_color_id::input_inactive_background));
      }
    } else if (widget.active) {
      auto const style = g_ctx->theme.get_style(im_color_id::input_active_text, im_color_id::input_active_background);
      auto const cursor_style = style.with_reverse();
      g_ctx->renderer.cmd_fill_rect(rect, ' ', style);

      auto content = std::span<std::uint32_t const>(text_input.text);
      if (password) {
        if (auto const masked = g_ctx->allocator.allocate<std::uint32_t>(content.size()); masked) {
          std::fill_n(masked, content.size(), password_ch);
          content = std::span<std::uint32_t const>(masked, content.size());
        }
      }
      auto const cursor_pos = static_cast<std::size_t>(text_input.cursor_pos);
      auto const display_width = rect.width();
      // all positions below are in terminal columns
      auto const content_width = text_width(content);
      auto const cursor_x = text_width(content.first(cursor_pos));
      auto const cursor_w = cursor_pos < content.size() ? char_width(content[cursor_pos]) : 1;

      if (content_width + 1 <= display_width) {
        text_input.scroll_offset = 0;
      } else {
        // keep cursor visible, scroll with small step to avoid jitter
        constexpr auto step = int(3);
        if (cursor_x < text_input.scroll_offset) {
          text_input.scroll_offset = cursor_x - step;
        } else if (cursor_x + cursor_w > text_input.scroll_offset + display_width) {
          text_input.scroll_offset = cursor_x + cursor_w - display_width + step;
        }
        text_input.scroll_offset = std::clamp(text_input.scroll_offset, 0, content_width + 1 - display_width);
      }

      auto const origin = rect.min - im_vec2(text_input.scroll_offset, 0);
      g_ctx->renderer.cmd_draw_text_at(origin, content, style);
      g_ctx->renderer.cmd_draw_text_at(origin + im_vec2(cursor_x, 0),
          cursor_pos < content.size() ? content.subspan(cursor_pos, 1) : space, cursor_style);
    } else {
      auto const style =
          g_ctx->theme.get_style(im_color_id::input_inactive_text, im_color_id::input_inactive_background);
      g_ctx->renderer.cmd_fill_rect(rect, ' ', style);
      auto content = std::span<std::uint32_t const>(to_unicode(input));
      if (password) {
        if (auto const masked = g_ctx->allocator.allocate<std::uint32_t>(content.size()); masked) {
          std::fill_n(masked, content.size(), password_ch);
          content = std::span<std::uint32_t const>(masked, content.size());
        }
      }
      g_ctx->renderer.cmd_draw_text_at(rect.min, content, style);
    }

    g_ctx->renderer.pop_clip_rect();
  }

  return widget.pressed;
}

auto checkbox(std::string_view label, bool& value) -> bool {
  auto& widget = g_ctx->widget;

  auto const [str, widget_key] = g_ctx->hash_id.split_str_key(label);
  auto const unicode_str = to_unicode(str);
  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(4 + text_width(unicode_str), 1));

  internal::common_focusable_behaviour(g_ctx->hash_id.make(widget_key), widget_rect);

  auto toggled = false;
  if (widget.active && (is_key_pressed(im_key_id::space) || is_key_pressed(im_key_id::enter) ||
                           widget.clicked_id == widget.current_id)) {
    value = !value;
    toggled = true;
  }

  if (g_ctx->renderer.is_visible(widget_rect)) {
    auto const bg = widget.active ? im_color_id::button_active_background : im_color_id::button_inactive_background;
    auto const fx_style = get_style(widget.active ? im_color_id::button_active_fx : im_color_id::button_inactive_fx, bg);
    auto const text_style =
        get_style(widget.active ? im_color_id::button_active_text : im_color_id::button_inactive_text, bg);

    static constexpr auto box = std::to_array<std::uint32_t>({'[', ' ', ']'});
    static constexpr auto mark = std::uint32_t('x');

    g_ctx->renderer.cmd_fill_rect(widget_rect, ' ', get_style_bg(bg));
    g_ctx->renderer.cmd_draw_text_at(widget_rect.min, box, fx_style);
    if (value) {
      g_ctx->renderer.cmd_draw_text_at(
          widget_rect.min + im_vec2(1, 0), std::span<std::uint32_t const>(&mark, 1), text_style);
    }
    g_ctx->renderer.cmd_draw_text_at(widget_rect.min + im_vec2(4, 0), unicode_str, text_style);
  }

  return toggled;
}

namespace {

// selection shared by list and table: rows_rect shows `rows` items starting at `offset`
auto selection_behaviour(im_rect const& rows_rect, int rows, int count, int& selected, int& offset) -> bool {
  auto const& widget = g_ctx->widget;
  selected = count > 0 ? std::clamp(selected, -1, count - 1) : -1;

  auto activated = false;
  if (widget.active && count > 0) {
    // click is mapped with offset user saw (previous frame)
    if (widget.clicked_id == widget.current_id && rows_rect.contains(widget.clicked_pos)) {
      if (auto const index = offset + (widget.clicked_pos.y - rows_rect.min.y); index >= 0 && index < count) {
        selected = index;
        activated = true;
      }
    }
    if (auto const delta = key_press_count(im_key_id::arrow_down) - key_press_count(im_key_id::arrow_up);
        delta != 0) {
      selected = std::clamp(selected < 0 ? (delta > 0 ? delta - 1 : 0) : selected + delta, 0, count - 1);
    }
    if (is_key_pressed(im_key_id::home)) {
      selected = 0;
    }
    if (is_key_pressed(im_key_id::end)) {
      selected = count - 1;
    }
    if (is_key_pressed(im_key_id::enter) && selected >= 0) {
      activated = true;
    }
  }

  // keep selection visible
  if (selected >= 0) {
    if (selected < offset) {
      offset = selected;
    } else if (selected >= offset + rows) {
      offset = selected - rows + 1;
    }
  }
  offset = std::clamp(offset, 0, std::max(0, count - rows));
  return activated;
}

[[nodiscard]] auto selected_row_style() -> im_style {
  return get_style(g_ctx->widget.active ? im_color_id::button_active_text : im_color_id::button_inactive_text,
      im_color_id::background)
      .with_reverse();
}

template <typename ItemAt>
auto list_impl(std::string_view label, int count, ItemAt&& item_at, int& selected, int height) -> bool {
  auto& widget = g_ctx->widget;

  auto const rows = std::max(1, height > 0 ? height : count);
  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(internal::item_width(fill()), rows));

  auto const [str, widget_key] = g_ctx->hash_id.split_str_key(label);
  internal::common_focusable_behaviour(g_ctx->hash_id.make(widget_key), widget_rect);

  auto& offset = g_ctx->list_scroll[widget.current_id];
  auto const activated = selection_behaviour(widget_rect, rows, count, selected, offset);
  if (widget.active && selected >= 0 && is_key_pressed(im_key_id::ctrl_c)) {
    set_clipboard(item_at(selected));
  }

  if (g_ctx->renderer.is_visible(widget_rect)) {
    auto const style = get_style(im_color_id::text, im_color_id::background);
    auto const selected_style = selected_row_style();

    g_ctx->renderer.cmd_fill_rect(widget_rect, ' ', style);
    for (int row = 0; row < rows && offset + row < count; ++row) {
      auto const index = offset + row;
      auto const y = widget_rect.min.y + row;
      auto const row_rect = im_rect(widget_rect.min.x, y, widget_rect.max.x, y);
      if (!g_ctx->renderer.is_visible(row_rect)) {
        continue;
      }
      auto const& row_style = index == selected ? selected_style : style;
      if (index == selected) {
        g_ctx->renderer.cmd_fill_rect(row_rect, ' ', row_style);
      }
      g_ctx->renderer.cmd_draw_text_at(row_rect.min, to_unicode(item_at(index)), row_style);
    }
  }

  return activated;
}

template <typename CellAt>
auto table_impl(std::string_view label, std::span<im_table_column const> columns, int count, CellAt&& cell_at,
    int& selected, int height) -> bool {
  auto& widget = g_ctx->widget;
  auto const ncols = int(columns.size());

  auto const rows = std::max(1, height > 0 ? height : count);
  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(internal::item_width(fill()), rows + 1));
  auto const rows_rect = widget_rect.crop_top(1);

  auto const [str, widget_key] = g_ctx->hash_id.split_str_key(label);
  internal::common_focusable_behaviour(g_ctx->hash_id.make(widget_key), widget_rect);

  auto& offset = g_ctx->list_scroll[widget.current_id];
  auto const activated = selection_behaviour(rows_rect, rows, count, selected, offset);
  if (widget.active && selected >= 0 && ncols > 0 && is_key_pressed(im_key_id::ctrl_c)) {
    // tab separated: pastes into spreadsheet columns
    auto row = std::string();
    for (int c = 0; c < ncols; ++c) {
      if (c > 0) {
        row += '\t';
      }
      row += cell_at(selected, c);
    }
    set_clipboard(row);
  }

  if (ncols == 0 || !g_ctx->renderer.is_visible(widget_rect)) {
    return activated;
  }

  // column widths: fixed and ratio first, fill() columns share the rest; 1 cell gap between columns
  auto widths = g_ctx->allocator.allocate<int>(std::size_t(ncols));
  if (!widths) [[unlikely]] {
    return activated;
  }
  auto const available = std::max(0, widget_rect.width() - (ncols - 1));
  auto used = 0, fills = 0;
  for (int c = 0; c < ncols; ++c) {
    auto const w = columns[std::size_t(c)].width;
    widths[c] = w < 0.0f ? 0 : w > 1.0f ? int(w) : int(std::ceil(w * float(available)));
    fills += w < 0.0f ? 1 : 0;
    used += widths[c];
  }
  for (int c = 0, rest = std::max(0, available - used); c < ncols; ++c) {
    if (columns[std::size_t(c)].width < 0.0f) {
      auto const leave = int(-columns[std::size_t(c)].width) - 1;
      widths[c] = std::max(0, rest / fills - leave);
    }
  }

  auto const align = [](im_align a) {
    return a == im_align::right ? im_halign::right : a == im_align::center ? im_halign::center : im_halign::left;
  };
  auto const draw_row = [&](int y, auto&& text_at, im_style const& style) {
    for (int c = 0, x = widget_rect.min.x; c < ncols && x <= widget_rect.max.x; ++c) {
      auto const cell = im_rect(x, y, std::min(x + widths[c] - 1, widget_rect.max.x), y);
      if (cell) {
        // text is cut by its column, not by the next one
        g_ctx->renderer.push_clip_rect(cell);
        g_ctx->renderer.cmd_draw_text_in_rect(
            cell, to_unicode(text_at(c)), style, align(columns[std::size_t(c)].align), im_valign::top);
        g_ctx->renderer.pop_clip_rect();
      }
      x += widths[c] + 1;
    }
  };

  auto const style = get_style(im_color_id::text, im_color_id::background);
  auto const selected_style = selected_row_style();
  auto const header_style = get_style(im_color_id::button_inactive_fx, im_color_id::background).with_underline();

  g_ctx->renderer.cmd_fill_rect(widget_rect, ' ', style);
  auto const header = im_rect(widget_rect.min.x, widget_rect.min.y, widget_rect.max.x, widget_rect.min.y);
  g_ctx->renderer.cmd_fill_rect(header, ' ', header_style);
  draw_row(widget_rect.min.y, [&](int c) { return columns[std::size_t(c)].title; }, header_style);

  for (int row = 0; row < rows && offset + row < count; ++row) {
    auto const index = offset + row;
    auto const y = rows_rect.min.y + row;
    if (!g_ctx->renderer.is_visible(im_rect(widget_rect.min.x, y, widget_rect.max.x, y))) {
      continue;
    }
    auto const& row_style = index == selected ? selected_style : style;
    if (index == selected) {
      g_ctx->renderer.cmd_fill_rect(im_rect(widget_rect.min.x, y, widget_rect.max.x, y), ' ', row_style);
    }
    draw_row(y, [&](int c) { return cell_at(index, c); }, row_style);
  }

  return activated;
}

} // namespace

auto table(std::string_view label, std::span<im_table_column const> columns, std::span<std::string_view const> cells,
    int& selected, int height) -> bool {
  auto const ncols = std::max<std::size_t>(1, columns.size());
  return table_impl(
      label, columns, int(cells.size() / ncols),
      [&](int r, int c) { return cells[std::size_t(r) * ncols + std::size_t(c)]; }, selected, height);
}

auto table(std::string_view label, std::span<im_table_column const> columns, std::span<std::string const> cells,
    int& selected, int height) -> bool {
  auto const ncols = std::max<std::size_t>(1, columns.size());
  return table_impl(
      label, columns, int(cells.size() / ncols),
      [&](int r, int c) { return std::string_view(cells[std::size_t(r) * ncols + std::size_t(c)]); }, selected,
      height);
}

auto list(std::string_view label, std::span<std::string_view const> items, int& selected, int height) -> bool {
  return list_impl(
      label, int(items.size()), [&](int i) { return items[std::size_t(i)]; }, selected, height);
}

auto list(std::string_view label, std::span<std::string const> items, int& selected, int height) -> bool {
  return list_impl(
      label, int(items.size()), [&](int i) { return std::string_view(items[std::size_t(i)]); }, selected, height);
}

namespace {

template <typename ItemAt>
auto tabs_impl(std::string_view label, int count, ItemAt&& item_at, int& selected) -> bool {
  auto& widget = g_ctx->widget;

  // segments " name " separated by one cell
  auto names = g_ctx->allocator.allocate<std::span<std::uint32_t const>>(std::size_t(std::max(count, 1)));
  if (!names) [[unlikely]] {
    return false;
  }
  auto total_width = 0;
  for (int i = 0; i < count; ++i) {
    names[i] = to_unicode(item_at(i));
    total_width += text_width(names[i]) + 2 + (i > 0 ? 1 : 0);
  }
  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(std::max(total_width, 1), 1));

  auto const [str, widget_key] = g_ctx->hash_id.split_str_key(label);
  internal::common_focusable_behaviour(g_ctx->hash_id.make(widget_key), widget_rect);

  auto const previous = selected = count > 0 ? std::clamp(selected, 0, count - 1) : -1;
  if (widget.active && count > 0) {
    if (widget.clicked_id == widget.current_id) {
      for (int i = 0, x = widget_rect.min.x; i < count; ++i) {
        auto const w = text_width(names[i]) + 2;
        if (widget.clicked_pos.x >= x && widget.clicked_pos.x < x + w) {
          selected = i;
        }
        x += w + 1;
      }
    }
    auto const delta = key_press_count(im_key_id::arrow_right) - key_press_count(im_key_id::arrow_left);
    selected = ((selected + delta) % count + count) % count;
  }

  if (g_ctx->renderer.is_visible(widget_rect)) {
    auto const style = get_style(im_color_id::button_inactive_text, im_color_id::background);
    auto const selected_style =
        get_style(widget.active ? im_color_id::button_active_text : im_color_id::button_inactive_text,
            im_color_id::background)
            .with_reverse();
    static constexpr auto space = std::uint32_t(' ');

    g_ctx->renderer.cmd_fill_rect(widget_rect, ' ', get_style_bg(im_color_id::background));
    for (int i = 0, x = widget_rect.min.x; i < count; ++i) {
      auto const w = text_width(names[i]) + 2;
      auto const& s = i == selected ? selected_style : style;
      g_ctx->renderer.cmd_fill_rect(im_rect(x, widget_rect.min.y, x + w - 1, widget_rect.min.y), space, s);
      g_ctx->renderer.cmd_draw_text_at(im_vec2(x + 1, widget_rect.min.y), names[i], s);
      x += w + 1;
    }
  }

  return selected != previous;
}

} // namespace

auto tabs(std::string_view label, std::span<std::string_view const> items, int& selected) -> bool {
  return tabs_impl(
      label, int(items.size()), [&](int i) { return items[std::size_t(i)]; }, selected);
}

auto tabs(std::string_view label, std::span<std::string const> items, int& selected) -> bool {
  return tabs_impl(
      label, int(items.size()), [&](int i) { return std::string_view(items[std::size_t(i)]); }, selected);
}

namespace {

constexpr auto spinner_update_interval = 0.1f; // 100ms
constexpr auto spinner_glyphs = std::to_array<std::uint32_t>({L'⣽', L'⣻', L'⢿', L'⡿', L'⣟', L'⣯', L'⣷'});

// TODO:
constexpr auto progress_glyph = std::to_array<std::uint32_t>({L'⣿', L'⡇'});

} // namespace

void spinner(std::string_view text, float& step) {
  static constexpr int spinner_min_width = 10;

  auto const unicode_text = to_unicode(text);
  auto const spinner_width = std::max<int>(spinner_min_width, text_width(unicode_text) + 2);
  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(spinner_width, 1));

  // update step
  step += g_ctx->elapsed;

  if (!g_ctx->renderer.is_visible(widget_rect)) {
    return;
  }

  auto const index = std::size_t(std::round(step / spinner_update_interval)) % spinner_glyphs.size();

  auto const style = g_ctx->theme.get_style(im_color_id::text, im_color_id::background);
  g_ctx->renderer.cmd_fill_rect(widget_rect, ' ', style);
  g_ctx->renderer.cmd_draw_text_at(widget_rect.min, std::span<std::uint32_t const>(&spinner_glyphs[index], 1), style);
  // glyph, space, text: matches spinner_width
  g_ctx->renderer.cmd_draw_text_at(widget_rect.min + im_vec2(2, 0), unicode_text, style);
}

void progress(float const& value) {
  static constexpr int progress_width = 10;

  auto const widget_rect = g_ctx->layout.add_widget_item(im_vec2(progress_width, 1));

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

  // integer ceil: partially used cell still holds pixels
  auto const width = (p_size.x + braille_pixels_per_width - 1) / braille_pixels_per_width;
  auto const height = (p_size.y + braille_pixels_per_height - 1) / braille_pixels_per_height;

  canvas.size = im_vec2(width, height);
  canvas.rect = g_ctx->layout.add_widget_item(canvas.size);

  // canvas_end() is called only when this returns true: push nothing when invisible
  if (!g_ctx->renderer.is_visible(canvas.rect)) {
    canvas.rect = {};
    canvas.size = {};
    canvas.data = {};
    return false;
  }

  auto const data_size = width * height;
  if (auto const data = g_ctx->allocator.allocate<im_cell>(data_size); data) {
    canvas.data = std::span<im_cell>(data, data_size);
  } else {
    canvas.data = {};
  }

  g_ctx->renderer.push_clip_rect(canvas.rect);

  auto const style = g_ctx->theme.get_style(im_color_id::text, im_color_id::background);
  if (!canvas.data.empty()) {
    // no background fill: canvas_end() draws every cell of the surface anyway
    std::fill(canvas.data.begin(), canvas.data.end(), im_cell{.ch = braille_offset, .style = style});
  } else {
    // allocation failed: nothing would be drawn, keep area clean
    g_ctx->renderer.cmd_fill_rect(canvas.rect, ' ', style);
  }

  return true;
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
  cell->style.fg = color.value;
}

} // namespace xxx
