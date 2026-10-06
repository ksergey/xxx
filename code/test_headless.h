// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "im_backend_headless.h"
#include "test_utils.h"
#include "xxx.h"

namespace xxx::testing {

// distinct colors for focused / unfocused elements, so tests don't depend on default palette
inline constexpr auto test_active = im_color(0xaa0000u);
inline constexpr auto test_inactive = im_color(0x00aa00u);

/// Library instance running on headless backend for the lifetime of the object
class headless_app {
public:
  explicit headless_app(im_vec2 size = im_vec2(40, 10)) {
    auto backend = std::make_unique<im_backend_headless>(size);
    backend_ = backend.get();
    xxx::init(std::move(backend));

    for (auto const role : {im_role::border_focused, im_role::title_focused, im_role::accent, im_role::input_focused}) {
      xxx::set_style(role, {.fg = test_active, .bg = {}, .attrs = 0});
    }
    for (auto const role : {im_role::border, im_role::title, im_role::muted, im_role::input}) {
      xxx::set_style(role, {.fg = test_inactive, .bg = {}, .attrs = 0});
    }
    // keep the attributes of the default theme: reverse means focus, underline means selection
    xxx::set_style(im_role::focus, {.fg = test_active, .bg = {}, .attrs = im_attr_reverse});
    xxx::set_style(im_role::selection, {.fg = test_inactive, .bg = {}, .attrs = im_attr_underline});
  }

  ~headless_app() {
    xxx::shutdown();
  }

  headless_app(headless_app const&) = delete;
  headless_app& operator=(headless_app const&) = delete;

  /// Run one full frame: input -> widgets -> render
  template <typename F>
  void frame(F&& build) {
    xxx::process_input_events();
    xxx::new_frame();
    build();
    xxx::render();
  }

  [[nodiscard]] auto backend() noexcept -> im_backend_headless& {
    return *backend_;
  }

  [[nodiscard]] auto screen() const -> std::string {
    return backend_->screen();
  }

  [[nodiscard]] auto line(int y) const -> std::string {
    return backend_->line(y);
  }

  /// Foreground color of presented cell
  [[nodiscard]] auto fg(int x, int y) const -> im_color {
    return im_color(backend_->cell(x, y).style.fg);
  }

  /// Is any cell of row y reversed (focused widget there)
  [[nodiscard]] auto reversed_in_row(int y) const -> bool {
    for (auto const p : reversed_cells()) {
      if (p.y == y) {
        return true;
      }
    }
    return false;
  }

  /// Cells drawn with underline attribute (selection of an unfocused list / table / tabs)
  [[nodiscard]] auto underlined_cells() const -> std::vector<im_vec2> {
    auto result = std::vector<im_vec2>();
    auto const size = backend_->size();
    for (int y = 0; y < size.y; ++y) {
      for (int x = 0; x < size.x; ++x) {
        if (backend_->cell(x, y).style.attrs & im_attr_underline) {
          result.emplace_back(x, y);
        }
      }
    }
    return result;
  }

  /// Cells drawn with reverse attribute: where the keys go (focused widget, text cursor)
  [[nodiscard]] auto reversed_cells() const -> std::vector<im_vec2> {
    auto result = std::vector<im_vec2>();
    auto const size = backend_->size();
    for (int y = 0; y < size.y; ++y) {
      for (int x = 0; x < size.x; ++x) {
        if (backend_->cell(x, y).style.attrs & im_attr_reverse) {
          result.emplace_back(x, y);
        }
      }
    }
    return result;
  }

private:
  im_backend_headless* backend_ = nullptr;
};

/// Snapshot helper: drop first newline, common indentation and trailing blank lines.
/// Allows writing expected screens as indented raw string literals.
[[nodiscard]] inline auto dedent(std::string_view text) -> std::string {
  if (text.starts_with('\n')) {
    text.remove_prefix(1);
  }
  auto lines = std::vector<std::string_view>();
  for (auto pos = std::size_t(0); pos <= text.size();) {
    auto const end = std::min(text.find('\n', pos), text.size());
    lines.push_back(text.substr(pos, end - pos));
    pos = end + 1;
  }
  auto indent = std::string_view::npos;
  for (auto const l : lines) {
    if (auto const first = l.find_first_not_of(' '); first != std::string_view::npos) {
      indent = std::min(indent, first);
    }
  }
  auto result = std::string();
  for (auto const l : lines) {
    auto s = l.size() > indent ? l.substr(indent) : std::string_view();
    s = s.substr(0, s.find_last_not_of(' ') + 1);
    result.append(s).push_back('\n');
  }
  result.erase(result.find_last_not_of('\n') + 1);
  return result;
}

} // namespace xxx::testing
