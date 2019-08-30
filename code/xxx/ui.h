#ifndef KSERGEY_ui_300819104733
#define KSERGEY_ui_300819104733

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string_view>

namespace xxx {

/// Colors.
enum class color : std::uint16_t { default_ = 0x00, black, red, green, yellow, blue, magenta, cyan, white };

/// Key event.
struct key {
  std::uint32_t ch;
  std::uint16_t key;
  std::uint8_t mod;
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

/// Init internals.
/// \throw std::runtime_error on error.
void init();

/// Shutdown internals.
void shutdown();

/// Update internal terminal state.
void update(unsigned ms = 33);

/// Set handler on key press event.
void set_key_event_handler(std::function<void(key const&)> handler);

/// Start drawing frame.
void begin();

/// Stop drawing frame.
void end();

/// Start drawing row.
/// @param[in] columns is number of columns inside row.
void row_begin(std::size_t columns);

/// Start next column in row.
/// @param[in] ratio_or_width is column percentage width (value <= 1.0) or explicit width.
void row_push(float ratio_or_width);

/// Stop drawing row.
void row_end();

/// Start drawing panel.
void panel_begin(std::string_view title = {});

/// Stop drawing panel.
void panel_end();

/// Add empty area.
/// \param[in] ratio_or_height is height percentage or explicit height.
void spacer(float ratio_or_height = 1.0);

/// Draw text line.
void text(std::string_view str, color fg = color::default_, color bg = color::default_);

/// Draw spinner.
void spinner(float& step_storage, std::string_view text = {});

}  // namespace xxx

#endif /* KSERGEY_ui_300819104733 */
