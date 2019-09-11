// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#ifndef KSERGEY_data_300819145443
#define KSERGEY_data_300819145443

#include <array>
#include <chrono>
#include <string>
#include <type_traits>
#include <vector>

#include <xxx/ui.h>

namespace xxx::impl {

struct size {
  int width{0};
  int height{0};
};

struct point {
  int x{0};
  int y{0};
};

struct rect {
  int x{0};
  int y{0};
  int width{0};
  int height{0};
};

namespace style {

/// Border style.
struct border {
  std::uint32_t vertical_line;
  std::uint32_t horizontal_line;
  std::uint32_t upper_left_corner;
  std::uint32_t upper_right_corner;
  std::uint32_t bottom_left_corner;
  std::uint32_t bottom_right_corner;
};

}  // namespace style

enum class layout_type { container, row, column };

struct layout_state {
  layout_type type{layout_type::container};
  impl::size size{};
  impl::point pos{};
  impl::size filled_size{};
  std::size_t columns{0};
  std::size_t column{0};
};

static_assert(std::is_trivially_copyable_v<layout_state>);

/// Clock for internal usage.
using clock = std::chrono::steady_clock;

/// UI global context.
struct context {
  struct {
    /// Panel widget style.
    struct {
      color title_color;
      color border_color;
      style::border border;
    } panel;

    /// Spinner widget style.
    struct {
      color spinner_color;
      color label_color;
      std::vector<std::uint32_t> glyphs;
    } spinner;

    /// Progress widget style.
    struct {
      color bar_color;
      color label_color;
      std::uint32_t bar_glyph;
    } progress;

    /// Text input widget.
    struct {
      color fg;
      color bg;
    } text_input;
  } style;

  /// Layout stack.
  std::vector<layout_state> layout_stack;

  /// Text input chars.
  std::u32string input_queue_chars;

  /// Pressed keys.
  std::array<bool, 512> pressed_keys;

  /// Screen size (updates once per cycle).
  impl::size screen_size{};

  /// Last clock timestamp.
  clock::time_point timestamp;

  /// Time step.
  float deltaTime{0.0};
};

}  // namespace xxx::impl

#endif /* KSERGEY_data_300819145443 */
