// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#ifndef KSERGEY_data_300819145443
#define KSERGEY_data_300819145443

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
  } style;

  /// Key-press handler.
  std::function<void(key const&)> key_event_handler;

  /// Layout stack.
  std::vector<layout_state> layout_stack;

  /// Text input chars.
  std::vector<char> input_queue_chars;

  /// Screen size (updates once per cycle).
  impl::size screen_size{};

  /// Time step.
  float deltaTime{0.0};
};

}  // namespace xxx::impl

#endif /* KSERGEY_data_300819145443 */
