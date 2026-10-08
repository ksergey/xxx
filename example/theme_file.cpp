// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "theme_file.h"

#include <array>
#include <charconv>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include <xxx.h>

namespace theme_file {

namespace {

using namespace xxx;
using json = nlohmann::json;
using namespace std::string_view_literals;

constexpr auto role_names = std::to_array<std::pair<std::string_view, im_role>>({
    {"text", im_role::text},
    {"muted", im_role::muted},
    {"accent", im_role::accent},
    {"border", im_role::border},
    {"border_focused", im_role::border_focused},
    {"title", im_role::title},
    {"title_focused", im_role::title_focused},
    {"focus", im_role::focus},
    {"selection", im_role::selection},
    {"header", im_role::header},
    {"input", im_role::input},
    {"input_focused", im_role::input_focused},
    {"placeholder", im_role::placeholder},
    {"error", im_role::error},
    {"warning", im_role::warning},
    {"success", im_role::success},
});
static_assert(role_names.size() == std::size_t(im_role::last), "every role has a name");

constexpr auto color_names = std::to_array<std::pair<std::string_view, std::uint8_t>>({
    {"black", 0},
    {"red", 1},
    {"green", 2},
    {"yellow", 3},
    {"blue", 4},
    {"magenta", 5},
    {"cyan", 6},
    {"white", 7},
    {"bright_black", 8},
    {"bright_red", 9},
    {"bright_green", 10},
    {"bright_yellow", 11},
    {"bright_blue", 12},
    {"bright_magenta", 13},
    {"bright_cyan", 14},
    {"bright_white", 15},
});

constexpr auto attr_names = std::to_array<std::pair<std::string_view, std::uint32_t>>({
    {"bold", im_attr_bold},
    {"dim", im_attr_dim},
    {"italic", im_attr_italic},
    {"underline", im_attr_underline},
    {"blink", im_attr_blink},
    {"reverse", im_attr_reverse},
    {"strikeout", im_attr_strikeout},
});

template <typename Table>
[[nodiscard]] auto lookup(Table const& table, std::string_view name) {
    using value_type = typename Table::value_type::second_type;
    for (auto const& [key, value] : table) {
        if (key == name) {
            return std::optional<value_type>(value);
        }
    }
    return std::optional<value_type>();
}

struct error {
    std::string message;
};

// color value or null (unset)
[[nodiscard]] auto parse_color(json const& value, std::string const& where) -> std::optional<im_color> {
    if (value.is_null()) {
        return std::nullopt;
    }
    if (value.is_number_integer()) {
        auto const index = value.get<long long>();
        if (index < 0 || index > 255) {
            throw error{std::format("{}: palette index {} is out of 0..255", where, index)};
        }
        return im_color::indexed(std::uint8_t(index));
    }
    if (!value.is_string()) {
        throw error{std::format("{}: color must be a string, a number 0..255 or null", where)};
    }
    auto const text = value.get<std::string>();
    if (text == "default") {
        return im_color();
    }
    if (auto const index = lookup(color_names, text)) {
        return im_color::indexed(*index);
    }
    if (text.size() == 7 && text[0] == '#') {
        auto rgb = std::uint32_t(0);
        auto const [ptr, ec] = std::from_chars(text.data() + 1, text.data() + 7, rgb, 16);
        if (ec == std::errc() && ptr == text.data() + 7) {
            return im_color(rgb);
        }
    }
    throw error{std::format(
        "{}: unknown color \"{}\" (use \"default\", a name like \"red\", \"#rrggbb\" or 0..255)", where, text)};
}

[[nodiscard]] auto parse_attrs(json const& value, std::string const& where) -> std::uint32_t {
    auto const one = [&](json const& item) {
        if (!item.is_string()) {
            throw error{std::format("{}: attribute must be a string", where)};
        }
        auto const attr = lookup(attr_names, item.get<std::string>());
        if (!attr) {
            throw error{std::format("{}: unknown attribute \"{}\"", where, item.get<std::string>())};
        }
        return *attr;
    };
    if (value.is_string()) {
        return one(value);
    }
    if (!value.is_array()) {
        throw error{std::format("{}: attrs must be a list of strings", where)};
    }
    auto attrs = std::uint32_t(0);
    for (auto const& item : value) {
        attrs |= one(item);
    }
    return attrs;
}

} // namespace

auto load_theme(std::string_view text) -> std::string {
    auto document = json();
    try {
        document = json::parse(text);
    } catch (json::parse_error const& e) {
        return std::format("invalid JSON: {}", e.what());
    }
    if (!document.is_object()) {
        return "theme must be a JSON object";
    }

    // a field of a role: absent, or a value (a color may be null: unset, follows "text")
    struct override_t {
        im_role role;
        std::optional<std::optional<im_color>> fg;
        std::optional<std::optional<im_color>> bg;
        std::optional<std::uint32_t> attrs;
    };

    try {
        // validate everything first: the theme is changed only when the whole file is fine
        auto base = std::optional<im_theme_preset>();
        if (document.contains("base")) {
            auto const& value = document["base"];
            if (value == "terminal") {
                base = im_theme_preset::terminal;
            } else if (value == "classic") {
                base = im_theme_preset::classic;
            } else {
                throw error{std::format("base: unknown theme {} (use \"terminal\" or \"classic\")", value.dump())};
            }
        }

        auto overrides = std::vector<override_t>();
        for (auto const& [key, value] : document.items()) {
            if (key == "base") {
                continue;
            }
            auto const role = lookup(role_names, key);
            if (!role) {
                throw error{std::format("unknown role \"{}\"", key)};
            }
            if (!value.is_object()) {
                throw error{std::format("{}: must be an object like {{\"fg\": \"red\"}}", key)};
            }
            auto& o = overrides.emplace_back(override_t{.role = *role, .fg = {}, .bg = {}, .attrs = {}});
            for (auto const& [field, field_value] : value.items()) {
                auto const where = key + "." + field;
                if (field == "fg") {
                    o.fg = parse_color(field_value, where);
                } else if (field == "bg") {
                    o.bg = parse_color(field_value, where);
                } else if (field == "attrs") {
                    o.attrs = parse_attrs(field_value, where);
                } else {
                    throw error{std::format("{}: unknown field (use fg, bg, attrs)", where)};
                }
            }
        }

        // apply through the public API; get_theme_style keeps unset colors unset (still following "text")
        if (base) {
            use_theme(*base);
        }
        for (auto const& o : overrides) {
            auto style = get_theme_style(o.role);
            if (o.fg) {
                style.fg = *o.fg;
            }
            if (o.bg) {
                style.bg = *o.bg;
            }
            if (o.attrs) {
                style.attrs = *o.attrs;
            }
            set_style(o.role, style);
        }
    } catch (error const& e) {
        return e.message;
    }
    return {};
}

auto load_theme_file(std::filesystem::path const& path) -> std::string {
    auto file = std::ifstream(path, std::ios::binary);
    if (!file) {
        return std::format("cannot read {}", path.string());
    }
    auto text = std::ostringstream();
    text << file.rdbuf();
    if (auto message = load_theme(text.str()); !message.empty()) {
        return std::format("{}: {}", path.string(), message);
    }
    return {};
}

} // namespace theme_file
