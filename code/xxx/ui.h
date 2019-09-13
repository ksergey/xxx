// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#ifndef KSERGEY_ui_300819104733
#define KSERGEY_ui_300819104733

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string_view>

#include <xxx/color.h>
#include <xxx/key.h>

namespace xxx {

/// Align.
enum class align { left, center, right };

/// Rect.
struct rect {
  int x{0};
  int y{0};
  int width{0};
  int height{0};
};

/// Init internals.
/// @throw std::runtime_error on error.
void init();

/// Shutdown internals.
void shutdown();

/// Update internal terminal state.
/// @return true on an tty event received.
bool update(unsigned ms = 33);

/// Return true on key pressed.
bool is_key_pressed(key k);

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

void fixed_panel_begin(rect const& geom, std::string_view title = {});
void fixed_panel_end();

/// Add empty area.
/// @param[in] ratio_or_height is height percentage or explicit height.
void spacer(float ratio_or_height = 1.0);

/// Draw text line.
void label(std::string_view text, color color = color::default_, align alignment = align::left);

/// Draw spinner.
void spinner(float& step, std::string_view text = {}, align alignment = align::left);

/// Draw progress bar.
void progress(float& value);

/// Draw text input.
bool text_input(std::string& input);

}  // namespace xxx

#endif /* KSERGEY_ui_300819104733 */
