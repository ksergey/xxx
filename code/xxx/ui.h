// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string_view>

#include <xxx/common.h>

namespace xxx {

/// Rect.
struct Rect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

/// Init internals.
/// @throw std::runtime_error on error.
void init();

/// Shutdown internals.
void shutdown();

/// Update internal terminal state.
/// @return true on an tty event received.
bool update(unsigned ms = 33) noexcept;

/// Return true on key pressed.
bool isKeyPressed(Key key) noexcept;

/// Start drawing frame.
void begin();

/// Stop drawing frame.
void end();

/// Start drawing row.
/// @param[in] columns is number of columns inside row.
void rowBegin(std::size_t columns);

/// Start next column in row.
/// @param[in] ratioOrWidth is column percentage width (value <= 1.0) or explicit width.
void rowPush(float ratioOrWidth);

/// Stop drawing row.
void rowEnd();

/// Start drawing panel.
void panelBegin(std::string_view title = {});

/// Stop drawing panel.
void panelEnd();

/// Start drawing fixed size panel
void fixedPanelBegin(Rect const& geom, std::string_view title = {});

/// Stop drawing fixed size panel
void fixedPanelEnd();

/// Add empty area.
/// @param[in] ratioOrHeight is height percentage or explicit height.
void spacer(float ratioOrHeight = 1.0);

/// Draw text line.
void label(std::string_view text, Color color = Color::Default, Align alignment = Align::Left);

/// Draw spinner.
void spinner(float& step, std::string_view text = {}, Align alignment = Align::Left);

/// Draw progress bar.
void progress(float& value);

/// Draw text input.
bool textInput(std::string& input);

} // namespace xxx
