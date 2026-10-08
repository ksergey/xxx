// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace xxx {

/// Standard base64 (RFC 4648) with padding
[[nodiscard]] constexpr auto base64_encode(std::string_view input) -> std::string {
    constexpr auto alphabet = std::string_view("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
    auto result = std::string();
    result.reserve((input.size() + 2) / 3 * 4);
    auto const byte = [&](std::size_t i) {
        return std::uint32_t(static_cast<unsigned char>(input[i]));
    };
    auto i = std::size_t(0);
    for (; i + 3 <= input.size(); i += 3) {
        auto const n = (byte(i) << 16) | (byte(i + 1) << 8) | byte(i + 2);
        result += alphabet[(n >> 18) & 63];
        result += alphabet[(n >> 12) & 63];
        result += alphabet[(n >> 6) & 63];
        result += alphabet[n & 63];
    }
    if (auto const rest = input.size() - i; rest > 0) {
        auto const n = (byte(i) << 16) | (rest == 2 ? byte(i + 1) << 8 : 0);
        result += alphabet[(n >> 18) & 63];
        result += alphabet[(n >> 12) & 63];
        result += rest == 2 ? alphabet[(n >> 6) & 63] : '=';
        result += '=';
    }
    return result;
}

static_assert(base64_encode("") == "");
static_assert(base64_encode("foobar") == "Zm9vYmFy");

/// OSC 52 sequence setting system clipboard to \c text.
/// Inside tmux both forms are sent (checked against tmux 3.4):
///   plain - accepted with "set -g set-clipboard on": tmux stores a buffer and forwards it outside;
///   DCS passthrough - reaches outer terminal with "set -g allow-passthrough on" (off by default).
/// If both work, outer terminal receives the same text twice, which is harmless.
[[nodiscard]] inline auto osc52_sequence(std::string_view text, bool tmux) -> std::string {
    auto const osc = "\x1b]52;c;" + base64_encode(text) + "\x07";
    if (!tmux) {
        return osc;
    }
    // DCS passthrough: every ESC inside is doubled
    auto wrapped = osc + "\x1bPtmux;";
    for (auto const ch : osc) {
        if (ch == '\x1b') {
            wrapped += '\x1b';
        }
        wrapped += ch;
    }
    wrapped += "\x1b\\";
    return wrapped;
}

} // namespace xxx
