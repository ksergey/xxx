// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cassert>

#include "im_renderer.h"
#include "im_stack.h"
#include "xxx.h"

namespace xxx {

/// Styles of the theme roles plus a stack of temporary overrides (push_style / pop_style)
class im_theme {
public:
    static constexpr auto roles_count = static_cast<std::size_t>(im_role::last);

    im_theme() {
        use(im_theme_preset::terminal);
    }

    /// Replace all roles with a built-in theme, drop overrides
    void use(im_theme_preset preset) {
        reset();
        roles_ = (preset == im_theme_preset::classic) ? classic() : terminal();
    }

    void set(im_role role, im_role_style const& style) noexcept {
        roles_[index(role)] = style;
    }

    [[nodiscard]] auto get(im_role role) const noexcept -> im_role_style const& {
        return roles_[index(role)];
    }

    /// Role style with unset colors taken from im_role::text
    [[nodiscard]] auto resolved(im_role role) const noexcept -> im_role_style {
        auto result = get(role);
        auto const& text = get(im_role::text);
        if (!result.fg) {
            result.fg = text.fg.value_or(im_color());
        }
        if (!result.bg) {
            result.bg = text.bg.value_or(im_color());
        }
        return result;
    }

    /// Ready to draw
    [[nodiscard]] auto style(im_role role) const noexcept -> im_style {
        auto const r = resolved(role);
        return im_style(*r.fg, *r.bg, r.attrs);
    }

    /// Ready to draw: a custom style, unset colors from im_role::text
    [[nodiscard]] auto style(im_role_style const& custom) const noexcept -> im_style {
        auto const& text = get(im_role::text);
        return im_style(custom.fg.value_or(text.fg.value_or(im_color())),
            custom.bg.value_or(text.bg.value_or(im_color())), custom.attrs);
    }

    void push(im_role role, im_role_style const& style) {
        stack_.push_back({.role = role, .previous = get(role)});
        set(role, style);
    }

    void pop(std::size_t count = 1) noexcept {
        for (; count > 0 && !stack_.empty(); --count) {
            auto const& saved = stack_.back();
            set(saved.role, saved.previous);
            stack_.pop_back();
        }
    }

    /// Undo overrides left over (called every frame)
    void reset() noexcept {
        pop(stack_.size());
    }

private:
    [[nodiscard]] static constexpr auto index(im_role role) noexcept -> std::size_t {
        assert(role < im_role::last);
        return static_cast<std::size_t>(role);
    }

    using roles = std::array<im_role_style, roles_count>;

    [[nodiscard]] static auto terminal() -> roles {
        auto r = roles();
        auto const set = [&](im_role role, im_role_style const& style) {
            r[index(role)] = style;
        };
        set(im_role::text, {.fg = im_color(), .bg = im_color(), .attrs = 0});
        set(im_role::muted, {.fg = ansi::bright_black, .bg = {}, .attrs = 0});
        set(im_role::accent, {.fg = ansi::cyan, .bg = {}, .attrs = 0});
        set(im_role::border, {});
        set(im_role::border_focused, {.fg = ansi::cyan, .bg = {}, .attrs = 0});
        set(im_role::title, {});
        set(im_role::title_focused, {.fg = ansi::cyan, .bg = {}, .attrs = im_attr_bold});
        set(im_role::focus, {.fg = ansi::cyan, .bg = {}, .attrs = im_attr_reverse});
        set(im_role::selection, {.fg = {}, .bg = {}, .attrs = im_attr_underline});
        set(im_role::header, {.fg = {}, .bg = {}, .attrs = im_attr_bold | im_attr_underline});
        set(im_role::input, {});
        set(im_role::input_focused, {.fg = ansi::cyan, .bg = {}, .attrs = 0});
        set(im_role::placeholder, {.fg = ansi::bright_black, .bg = {}, .attrs = 0});
        set(im_role::error, {.fg = ansi::red, .bg = {}, .attrs = 0});
        set(im_role::warning, {.fg = ansi::yellow, .bg = {}, .attrs = 0});
        set(im_role::success, {.fg = ansi::green, .bg = {}, .attrs = 0});
        return r;
    }

    [[nodiscard]] static auto classic() -> roles {
        auto r = roles();
        auto const set = [&](im_role role, im_role_style const& style) {
            r[index(role)] = style;
        };
        constexpr auto accent = im_color(0xdcf763u);
        constexpr auto gray = im_color(0x848c8eu);
        set(im_role::text, {.fg = im_color(), .bg = im_color(), .attrs = 0});
        set(im_role::muted, {.fg = gray, .bg = {}, .attrs = 0});
        set(im_role::accent, {.fg = accent, .bg = {}, .attrs = 0});
        set(im_role::border, {.fg = im_color(0xbfb7b6u), .bg = {}, .attrs = 0});
        set(im_role::border_focused, {.fg = accent, .bg = {}, .attrs = 0});
        set(im_role::title, {});
        set(im_role::title_focused, {.fg = accent, .bg = {}, .attrs = 0});
        set(im_role::focus, {.fg = accent, .bg = {}, .attrs = im_attr_reverse});
        set(im_role::selection, {.fg = gray, .bg = {}, .attrs = im_attr_underline});
        set(im_role::header, {.fg = im_color(0xbfb7b6u), .bg = {}, .attrs = im_attr_underline});
        set(im_role::input, {.fg = gray, .bg = {}, .attrs = 0});
        set(im_role::input_focused, {.fg = accent, .bg = {}, .attrs = 0});
        set(im_role::placeholder, {.fg = gray, .bg = {}, .attrs = 0});
        set(im_role::error, {.fg = im_color(0xfb4934u), .bg = {}, .attrs = 0});
        set(im_role::warning, {.fg = im_color(0xfabd2fu), .bg = {}, .attrs = 0});
        set(im_role::success, {.fg = im_color(0x8ec07cu), .bg = {}, .attrs = 0});
        return r;
    }

    struct saved_style {
        im_role role;
        im_role_style previous;
    };

    roles roles_;
    im_stack<saved_style> stack_ = im_stack<saved_style>(32);
};

} // namespace xxx
