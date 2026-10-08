// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

#include "im_renderer.h"
#include "im_vec2.h"

namespace xxx {

/// Double buffered screen producing xterm escape sequences.
/// Pure: present() appends the bytes to send; nothing touches the terminal here.
/// Only changed cells are written; a frame is wrapped into synchronized output (DEC 2026),
/// so terminals supporting it never show half drawn frames (others ignore it).
class ansi_screen {
public:
    /// What the terminal can show: colors it can't are mapped to the nearest one it can
    enum class color_mode { none, ansi16, palette256, truecolor };

    void set_color_mode(color_mode mode) noexcept {
        mode_ = mode;
        invalidate();
    }

    /// Change size; next present() redraws everything
    void resize(im_vec2 size);

    [[nodiscard]] auto size() const noexcept -> im_vec2 {
        return size_;
    }

    /// Fill back buffer with spaces of \c style
    void clear(im_style const& style);

    /// Set back buffer cell; out of screen coordinates are ignored
    void set_cell(int x, int y, std::uint32_t ch, im_style const& style);

    /// Append escape sequences turning shown frame into back buffer to \c out
    void present(std::string& out);

    /// Forget what terminal shows: next present() redraws everything
    void invalidate();

private:
    void emit_style(im_style style, std::string& out);

    // style as it is sent: without colors in color_mode::none
    [[nodiscard]] auto effective(im_style style) const noexcept -> im_style {
        if (mode_ == color_mode::none) {
            style.fg = style.bg = im_color::default_tag;
        }
        return style;
    }

    im_vec2 size_;
    std::vector<im_cell> back_;
    std::vector<im_cell> front_; // what terminal shows
    im_style last_style_;        // valid while last_style_known_
    bool last_style_known_ = false;
    color_mode mode_ = color_mode::truecolor;
};

} // namespace xxx
