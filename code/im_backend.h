// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>

#include "im_input.h"
#include "im_renderer.h"
#include "im_vec2.h"

namespace xxx {

using im_clock = std::chrono::steady_clock;

/// Terminal abstraction: input source and cell output
class im_backend {
public:
    virtual ~im_backend() = default;

    /// Screen size in cells
    [[nodiscard]] virtual auto size() const -> im_vec2 = 0;

    /// Current time, used for frame delta
    [[nodiscard]] virtual auto now() const -> im_clock::time_point {
        return im_clock::now();
    }

    /// Move pending input events into \c input.
    /// Waits up to \c timeout for the first event: 0 - don't wait, negative - wait forever.
    /// @return true if any event (key, mouse, resize) arrived
    virtual auto poll_events(im_input& input, std::chrono::milliseconds timeout) -> bool = 0;

    /// Start new frame: fill back buffer with spaces using \c style
    virtual void clear(im_style const& style) = 0;

    /// Set cell in back buffer. Wide char (char_width(ch) == 2) covers next cell too.
    /// Out of screen coordinates are ignored.
    virtual void set_cell(int x, int y, std::uint32_t ch, im_style const& style) = 0;

    /// Show back buffer
    virtual void present() = 0;

    /// Put utf8 text into system clipboard (best effort, may be unsupported)
    virtual void set_clipboard([[maybe_unused]] std::string_view text) {}

    /// Thread-safe: make a waiting poll_events() return (it reports an event)
    virtual void wake_up() = 0;
};

class terminal;

/// Terminal backend for xterm compatible terminals: owns the terminal, waits for events itself
[[nodiscard]] auto make_ansi_backend() -> std::unique_ptr<im_backend>;

/// Backend on an external terminal: never waits, takes the events queued in it
[[nodiscard]] auto make_terminal_backend(terminal& external) -> std::unique_ptr<im_backend>;

/// Init library with custom backend
void init(std::unique_ptr<im_backend> backend);

} // namespace xxx
