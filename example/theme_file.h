// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

// Example code (not part of the library, which has no dependencies): xxx themes from JSON with
// nlohmann_json, through the public theme API (use_theme, set_style, get_theme_style).
//
//   {
//     "base": "terminal",                                  optional: start from a built-in theme
//     "accent": { "fg": "bright_magenta", "attrs": ["bold"] },
//     "focus":  { "fg": "black", "bg": "#dcf763" },
//     "text":   { "bg": 236 }
//   }
//
// Keys are role names (text, muted, accent, border, border_focused, title, title_focused, focus,
// selection, header, input, input_focused, placeholder, error, warning, success).
// Colors: "default" (terminal color), ANSI names ("red", "bright_black", ...), "#rrggbb",
// or a palette index 0-255; null unsets a color (it comes from "text" again).
// Attributes: bold, dim, italic, underline, blink, reverse, strikeout.
// Fields given for a role replace the current ones, the rest stay.

namespace theme_file {

/// Apply a theme from JSON text: all or nothing. Call after xxx::init().
/// @return empty string on success, otherwise what is wrong (the theme is not changed then)
[[nodiscard]] auto load_theme(std::string_view json) -> std::string;

/// load_theme() from a file
[[nodiscard]] auto load_theme_file(std::filesystem::path const& path) -> std::string;

} // namespace theme_file
