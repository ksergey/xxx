// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "unicode.h"

#include <algorithm>

#include <iterator>

#include "unicode_width_table.h"

namespace xxx {

auto utf8_decode(std::string_view input, std::uint32_t& ch) noexcept -> std::size_t {
    if (input.empty() || input[0] == '\0') {
        return 0;
    }
    auto const byte = [&](std::size_t i) {
        return std::uint32_t(static_cast<unsigned char>(input[i]));
    };
    auto const b0 = byte(0);
    if (b0 < 0x80) {
        ch = b0;
        return 1;
    }

    auto length = std::size_t(0);
    auto min = std::uint32_t(0);
    if ((b0 & 0xe0) == 0xc0) {
        length = 2, min = 0x80, ch = b0 & 0x1f;
    } else if ((b0 & 0xf0) == 0xe0) {
        length = 3, min = 0x800, ch = b0 & 0x0f;
    } else if ((b0 & 0xf8) == 0xf0) {
        length = 4, min = 0x10000, ch = b0 & 0x07;
    } else {
        // stray continuation byte or invalid lead byte
        ch = replacement_char;
        return 1;
    }
    if (input.size() < length) {
        // truncated: maybe the rest arrives later, stop here
        return 0;
    }
    for (auto i = std::size_t(1); i < length; ++i) {
        if ((byte(i) & 0xc0) != 0x80) {
            ch = replacement_char;
            return 1; // resync on the next byte
        }
        ch = (ch << 6) | (byte(i) & 0x3f);
    }
    if (ch < min || ch > 0x10ffff || (ch >= 0xd800 && ch <= 0xdfff)) {
        ch = replacement_char;
        return 1;
    }
    return length;
}

auto utf8_encode(std::uint32_t ch, char* out) noexcept -> std::size_t {
    if (ch > 0x10ffff || (ch >= 0xd800 && ch <= 0xdfff)) {
        ch = replacement_char;
    }
    if (ch < 0x80) {
        out[0] = char(ch);
        return 1;
    }
    if (ch < 0x800) {
        out[0] = char(0xc0 | (ch >> 6));
        out[1] = char(0x80 | (ch & 0x3f));
        return 2;
    }
    if (ch < 0x10000) {
        out[0] = char(0xe0 | (ch >> 12));
        out[1] = char(0x80 | ((ch >> 6) & 0x3f));
        out[2] = char(0x80 | (ch & 0x3f));
        return 3;
    }
    out[0] = char(0xf0 | (ch >> 18));
    out[1] = char(0x80 | ((ch >> 12) & 0x3f));
    out[2] = char(0x80 | ((ch >> 6) & 0x3f));
    out[3] = char(0x80 | (ch & 0x3f));
    return 4;
}

auto utf8_to_unicode(std::string_view input) -> std::span<std::uint32_t const> {
    thread_local std::vector<std::uint32_t> cache;
    utf8_to_unicode(input, cache);
    return cache;
}

void utf8_to_unicode(std::string_view input, std::vector<std::uint32_t>& output) {
    output.clear();
    for_each_codepoint(input, [&](std::uint32_t ch) {
        output.push_back(ch);
    });
}

[[nodiscard]] auto unicode_to_utf8(std::span<std::uint32_t const> input) -> std::string_view {
    thread_local std::string cache;
    unicode_to_utf8(input, cache);
    return cache;
}

void unicode_to_utf8(std::span<std::uint32_t const> input, std::string& output) {
    output.clear();
    char buffer[4];
    for (auto const ch : input) {
        output.append(buffer, utf8_encode(ch, buffer));
    }
}

auto char_width(std::uint32_t ch) noexcept -> int {
    // nothing below U+1100 is wide: covers ASCII, Latin, Cyrillic, box drawing fast
    if (ch < detail::wide_ranges.front().first) [[likely]] {
        return 1;
    }
    auto const it = std::upper_bound(detail::wide_ranges.begin(), detail::wide_ranges.end(), ch,
        [](std::uint32_t value, detail::width_range const& r) {
            return value < r.first;
        });
    return (it != detail::wide_ranges.begin() && ch <= std::prev(it)->last) ? 2 : 1;
}

auto text_width(std::span<std::uint32_t const> text) noexcept -> int {
    auto width = 0;
    for (auto const ch : text) {
        width += char_width(ch);
    }
    return width;
}

auto slice_columns(std::span<std::uint32_t const> text, int skip, int max_width) noexcept -> text_slice {
    auto result = text_slice();
    if (max_width <= 0) {
        return result;
    }
    skip = std::max(skip, 0);

    auto const size = text.size();
    auto i = std::size_t(0);
    auto column = 0;

    // drop leading columns
    while (i < size && column + char_width(text[i]) <= skip) {
        column += char_width(text[i++]);
    }
    auto used = 0;
    if (i < size && column < skip) {
        // wide char cut by left edge
        column += char_width(text[i++]);
        result.pad_left = 1;
        used = 1;
    }

    // take columns which fit
    auto const first = i;
    while (i < size && used + char_width(text[i]) <= max_width) {
        used += char_width(text[i++]);
    }
    result.text = text.subspan(first, i - first);

    if (i < size && used < max_width) {
        // wide char cut by right edge
        result.pad_right = 1;
    }
    return result;
}

} // namespace xxx
