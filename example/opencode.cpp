// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// A chat UI in the style of opencode (https://github.com/sst/opencode), built from the look pieces of xxx:
//
//   im_view_look            frameless panels with a background, padding and a "┃" bar (prompt, header)
//   set_popup_look          frameless dialogs over a darkened screen (the command palette, F1 help)
//   overlay_begin / _end    panels anywhere on the screen: the sidebar and toasts
//   scoped_style            a background for a group of labels (user messages, the sidebar card)
//   label({...}) parts      text in several styles on one line (status bar, keys)
//
// The "assistant" is fake: it streams a canned answer, no network involved.
//
// Keys: Enter send, Tab switch agent, Ctrl+P commands, PgUp / PgDn scroll, Esc interrupt, Ctrl+Q quit

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <xxx.h>

namespace {

using namespace std::chrono_literals;
using clock_type = std::chrono::steady_clock;
using xxx::im_role;
using xxx::im_text;

// ---------------------------------------------------------------------------------------------------------
// Look: opencode's dark palette mapped onto the theme roles
// ---------------------------------------------------------------------------------------------------------

namespace color {
constexpr auto background = xxx::im_color(0x0a0a0au);
constexpr auto panel = xxx::im_color(0x141414u);   // panels, sidebar, dialogs
constexpr auto element = xxx::im_color(0x1e1e1eu); // prompt, cards, toasts
constexpr auto text = xxx::im_color(0xeeeeeeu);
constexpr auto muted = xxx::im_color(0x808080u);
constexpr auto primary = xxx::im_color(0xfab283u); // selection, list bullets
constexpr auto blue = xxx::im_color(0x5c9cf5u);
constexpr auto purple = xxx::im_color(0x9d7cd8u); // headings
constexpr auto red = xxx::im_color(0xe06c75u);
constexpr auto green = xxx::im_color(0x7fd88fu);
constexpr auto border = xxx::im_color(0x484848u);
} // namespace color

void apply_theme() {
    using namespace xxx;
    set_style(im_role::text, {.fg = color::text, .bg = color::background, .attrs = 0});
    set_style(im_role::muted, {.fg = color::muted, .bg = {}, .attrs = 0});
    set_style(im_role::accent, {.fg = color::primary, .bg = {}, .attrs = 0});
    set_style(im_role::border, {.fg = color::border, .bg = {}, .attrs = 0});
    set_style(im_role::border_focused, {.fg = color::blue, .bg = {}, .attrs = 0});
    set_style(im_role::title, {.fg = color::muted, .bg = {}, .attrs = im_attr_bold});
    set_style(im_role::title_focused, {.fg = color::purple, .bg = {}, .attrs = im_attr_bold});
    set_style(im_role::focus, {.fg = color::background, .bg = color::primary, .attrs = im_attr_bold});
    set_style(im_role::selection, {.fg = {}, .bg = color::element, .attrs = 0});
    set_style(im_role::header, {.fg = color::muted, .bg = {}, .attrs = 0});
    set_style(im_role::input, {.fg = color::text, .bg = {}, .attrs = 0});
    set_style(im_role::input_focused, {.fg = color::text, .bg = {}, .attrs = 0});
    set_style(im_role::placeholder, {.fg = color::muted, .bg = {}, .attrs = 0});
    set_style(im_role::error, {.fg = color::red, .bg = {}, .attrs = 0});
    set_style(im_role::success, {.fg = color::green, .bg = {}, .attrs = 0});

    set_view_style(im_border::rounded, im_border::rounded, im_align::left);
    // dialogs: no frame, title inside with "esc" on the right, the screen behind darkened
    set_popup_look(
        {.border = im_border::none, .bg = color::panel, .pad_x = 3, .pad_y = 1, .title_gap = 1, .backdrop = 0.6f});
    enable_esc_back(false); // Esc interrupts the answer
}

constexpr auto sidebar_width = 42;
constexpr auto sidebar_min_screen = 100;

// ---------------------------------------------------------------------------------------------------------
// Text helpers (the demo text is ASCII plus a few one-cell symbols)
// ---------------------------------------------------------------------------------------------------------

[[nodiscard]] auto width(std::string_view s) -> int {
    return int(std::ranges::count_if(s, [](char c) {
        return (static_cast<unsigned char>(c) & 0xc0) != 0x80;
    }));
}

[[nodiscard]] auto wrap(std::string_view text, int max_width) -> std::vector<std::string> {
    auto lines = std::vector<std::string>();
    auto line = std::string();
    while (!text.empty()) {
        auto const space = text.find(' ');
        auto const word = text.substr(0, space);
        text = space == std::string_view::npos ? std::string_view() : text.substr(space + 1);
        if (!line.empty() && width(line) + 1 + width(word) > max_width) {
            lines.push_back(std::move(line));
            line.clear();
        }
        line += line.empty() ? "" : " ";
        line += word;
    }
    lines.push_back(std::move(line));
    return lines;
}

[[nodiscard]] auto lower(std::string_view s) -> std::string {
    auto result = std::string(s);
    for (auto& c : result) {
        c = char(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

// opencode's busy indicator: three squares running back and forth over six cells
[[nodiscard]] auto activity() -> std::string {
    auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now().time_since_epoch());
    auto const step = int(ms.count() / 120 % 6);
    auto const pos = step <= 3 ? step : 6 - step;
    auto result = std::string();
    for (int i = 0; i < 6; ++i) {
        result += (i >= pos && i < pos + 3) ? "■" : "•";
    }
    return result;
}

// ---------------------------------------------------------------------------------------------------------
// The conversation
// ---------------------------------------------------------------------------------------------------------

struct agent {
    std::string_view name;
    xxx::im_color color;
};
constexpr auto agents = std::to_array<agent>({{"Build", color::blue}, {"Plan", color::purple}});

struct message {
    enum class kind { user, assistant, tool, footer } type;
    std::string text;
    int agent = 0;
};

constexpr auto canned_answer = std::to_array<std::string_view>({
    "~Read example/opencode.cpp",
    "~Grep \"overlay_begin\" code/",
    "The screen is made of a few independent pieces, each drawn every frame:",
    "- the session: messages wrapped to the width of the column, the latest at the bottom",
    "- the prompt: a frameless view with a bar in the color of the agent",
    "- the sidebar: an overlay at the right edge, with a card at the bottom",
    "- the status bar: activity and keys on the left, context usage on the right",
    "Nothing keeps state between frames except the conversation itself: immediate mode all the way.",
});

class chat {
public:
    void send(std::string text, int agent) {
        messages_.push_back({.type = message::kind::user, .text = std::move(text), .agent = agent});
        tokens_ += 900 + int(messages_.size()) * 37;
        answer_line_ = 0;
        answer_chars_ = 0;
        started_ = clock_type::now();
        agent_ = agent;
        busy_ = true;
    }

    void interrupt() {
        if (busy_) {
            busy_ = false;
            messages_.push_back({.type = message::kind::footer, .text = "interrupted", .agent = agent_});
        }
    }

    void clear() {
        messages_.clear();
        busy_ = false;
        tokens_ = 0;
    }

    /// stream the answer: a few characters per tick
    void tick() {
        if (!busy_) {
            return;
        }
        if (answer_line_ >= int(canned_answer.size())) {
            busy_ = false;
            auto const seconds = std::chrono::duration<double>(clock_type::now() - started_).count();
            messages_.push_back({.type = message::kind::footer,
                .text = std::format("{} · demo-model · {:.1f}s", agents[std::size_t(agent_)].name, seconds),
                .agent = agent_});
            tokens_ += 1500;
            return;
        }
        auto const line = canned_answer[std::size_t(answer_line_)];
        if (line.starts_with('~')) { // a tool call: shows up at once
            messages_.push_back({.type = message::kind::tool, .text = std::string(line.substr(1)), .agent = agent_});
            ++answer_line_;
            return;
        }
        if (answer_chars_ == 0) {
            messages_.push_back({.type = message::kind::assistant, .text = {}, .agent = agent_});
        }
        answer_chars_ = std::min(int(line.size()), answer_chars_ + 6);
        messages_.back().text = std::string(line.substr(0, std::size_t(answer_chars_)));
        tokens_ += 2;
        if (answer_chars_ == int(line.size())) {
            ++answer_line_;
            answer_chars_ = 0;
        }
    }

    [[nodiscard]] auto messages() const -> std::vector<message> const& {
        return messages_;
    }
    [[nodiscard]] auto busy() const -> bool {
        return busy_;
    }
    [[nodiscard]] auto tokens() const -> int {
        return tokens_;
    }

private:
    std::vector<message> messages_;
    bool busy_ = false;
    int agent_ = 0;
    int answer_line_ = 0;
    int answer_chars_ = 0;
    int tokens_ = 0;
    clock_type::time_point started_;
};

// ---------------------------------------------------------------------------------------------------------
// Session: messages turned into screen rows, the last ones that fit are shown
// ---------------------------------------------------------------------------------------------------------

struct row {
    enum class kind { blank, user, user_pad, text, bullet, tool, footer } type = kind::blank;
    std::string text;
    int agent = 0;
};

[[nodiscard]] auto layout_rows(std::vector<message> const& messages, int text_width) -> std::vector<row> {
    auto rows = std::vector<row>();
    auto previous = std::optional<message::kind>();
    for (auto const& m : messages) {
        auto const add = [&](row::kind type, std::string text) {
            rows.push_back({.type = type, .text = std::move(text), .agent = m.agent});
        };
        // a blank line between blocks; consecutive answer lines and tool calls stay together
        auto const same_block =
            previous && *previous == m.type && (m.type == message::kind::assistant || m.type == message::kind::tool);
        if (previous && !same_block) {
            add(row::kind::blank, {});
        }
        switch (m.type) {
        case message::kind::user:
            add(row::kind::user_pad, {});
            for (auto& line : wrap(m.text, text_width - 4)) {
                add(row::kind::user, std::move(line));
            }
            add(row::kind::user_pad, {});
            break;
        case message::kind::assistant:
            if (m.text.starts_with("- ")) {
                auto first = true;
                for (auto& line : wrap(m.text.substr(2), text_width - 4)) {
                    add(first ? row::kind::bullet : row::kind::text, "  " + line);
                    first = false;
                }
            } else {
                for (auto& line : wrap(m.text, text_width - 2)) {
                    add(row::kind::text, std::move(line));
                }
            }
            break;
        case message::kind::tool: add(row::kind::tool, m.text); break;
        case message::kind::footer: add(row::kind::footer, m.text); break;
        }
        previous = m.type;
    }
    return rows;
}

void draw_row(row const& r) {
    // agent color for the bar and the footer square: a temporary accent
    auto const agent_color =
        xxx::scoped_style(im_role::accent, {.fg = agents[std::size_t(r.agent)].color, .bg = {}, .attrs = 0});
    switch (r.type) {
    case row::kind::blank: xxx::label(" "); break;
    case row::kind::user:
    case row::kind::user_pad: {
        // a user message is a block with its own background and a bar on the left
        auto const block = xxx::scoped_style(im_role::text, {.fg = color::text, .bg = color::panel, .attrs = 0});
        xxx::set_next_item_width(xxx::fill());
        xxx::label({{.text = "┃", .role = im_role::accent}, {.text = "  "}, {.text = r.text}});
    } break;
    case row::kind::text: xxx::label({{.text = "  "}, {.text = r.text}}); break;
    case row::kind::bullet:
        xxx::label({{.text = "  "}, {.text = "•", .role = im_role::accent, .attrs = 0}, {.text = r.text.substr(1)}});
        break;
    case row::kind::tool:
        xxx::label({{.text = "  "}, {.text = "→ ", .role = im_role::muted}, {.text = r.text, .role = im_role::muted}});
        break;
    case row::kind::footer:
        xxx::label({{.text = "  "}, {.text = "▣ ", .role = im_role::accent}, {.text = r.text, .role = im_role::muted}});
        break;
    }
}

// ---------------------------------------------------------------------------------------------------------
// Toasts: an overlay in the top right corner with bars in the color of the message
// ---------------------------------------------------------------------------------------------------------

class toasts {
public:
    void push(std::string text, bool error = false) {
        text_ = std::move(text);
        error_ = error;
        until_ = clock_type::now() + 3s;
    }

    [[nodiscard]] auto timeout() const -> std::optional<std::chrono::milliseconds> {
        auto const left = std::chrono::duration_cast<std::chrono::milliseconds>(until_ - clock_type::now());
        return text_.empty() || left <= 0ms ? std::nullopt : std::optional(left);
    }

    void draw(int right) const {
        if (!timeout()) {
            return;
        }
        auto const w = width(text_) + 6; // bars and padding
        auto const bars = xxx::scoped_style(
            im_role::border_focused, {.fg = error_ ? color::red : color::green, .bg = {}, .attrs = 0});
        xxx::overlay_begin(xxx::im_vec2(right - w + 1, 1), w,
            {.border = {}, .bg = color::element, .pad_x = 2, .pad_y = 1, .bar_left = true, .bar_right = true});
        xxx::label(text_);
        xxx::overlay_end();
    }

private:
    std::string text_;
    bool error_ = false;
    clock_type::time_point until_;
};

// ---------------------------------------------------------------------------------------------------------
// Command palette: a popup with a filter input; the list is drawn with labels, arrows are read directly
// (the input is the only focusable widget, so it keeps the keyboard)
// ---------------------------------------------------------------------------------------------------------

struct command {
    std::string_view category;
    std::string_view title;
    std::string_view keys;
    int id;
};

class palette {
public:
    void open() {
        filter_.clear();
        selected_ = 0;
        xxx::open_popup("commands");
    }

    [[nodiscard]] auto draw(std::span<command const> commands) -> std::optional<int> {
        auto const screen = xxx::get_screen_rect();
        if (!xxx::popup_begin("commands", "Commands", std::min(64, screen.width() - 4))) {
            return std::nullopt;
        }
        // the input: a row with a bar, on a lighter background
        auto submit = false;
        {
            auto const field = xxx::scoped_style(im_role::text, {.fg = color::text, .bg = color::element, .attrs = 0});
            xxx::layout_row_begin(2);
            xxx::layout_row_push(xxx::cells(2));
            xxx::label({{.text = "┃", .role = im_role::border_focused}});
            xxx::layout_row_push(xxx::fill());
            xxx::set_next_item_width(xxx::fill());
            submit = xxx::text_input("Search", filter_, xxx::im_input_flag_no_prompt);
            xxx::layout_row_end();
        }
        xxx::label(" ");

        auto const needle = lower(filter_);
        auto matches = std::vector<command const*>();
        for (auto const& c : commands) {
            if (lower(c.title).find(needle) != std::string::npos ||
                lower(c.category).find(needle) != std::string::npos) {
                matches.push_back(&c);
            }
        }
        auto const delta =
            xxx::key_press_count(xxx::im_key_id::arrow_down) - xxx::key_press_count(xxx::im_key_id::arrow_up);
        selected_ = matches.empty() ? 0 : std::clamp(selected_ + delta, 0, int(matches.size()) - 1);

        if (matches.empty()) {
            xxx::label("no matching commands", im_role::muted);
        }
        auto category = std::string_view();
        for (std::size_t i = 0; i < matches.size(); ++i) {
            auto const& c = *matches[i];
            if (c.category != category) {
                if (!category.empty()) {
                    xxx::label(" ");
                }
                category = c.category;
                xxx::label({{.text = category, .role = im_role::title_focused}});
            }
            // the selected row: dark text on peach, keys included
            auto const selected = int(i) == selected_;
            auto const row_style =
                selected ? xxx::im_role_style{.fg = color::background, .bg = color::primary, .attrs = xxx::im_attr_bold}
                         : xxx::im_role_style{.fg = color::text, .bg = color::panel, .attrs = 0};
            auto const text = xxx::scoped_style(im_role::text, row_style);
            auto const keys = xxx::scoped_style(
                im_role::muted, {.fg = selected ? color::background : color::muted, .bg = {}, .attrs = 0});
            auto const keys_width = width(c.keys) + 1;
            xxx::layout_row_begin(2);
            xxx::layout_row_push(xxx::fill(keys_width));
            xxx::set_next_item_width(xxx::fill());
            xxx::label({{.text = " "}, {.text = c.title}});
            xxx::layout_row_push(xxx::cells(keys_width));
            xxx::label({{.text = c.keys, .role = im_role::muted}, {.text = " "}}, xxx::im_align::right);
            xxx::layout_row_end();
        }
        xxx::label(" ");
        xxx::label({{.text = "↑↓", .attrs = xxx::im_attr_bold}, {.text = " select   ", .role = im_role::muted},
            {.text = "enter", .attrs = xxx::im_attr_bold}, {.text = " run", .role = im_role::muted}});
        xxx::popup_end();

        if (submit && !matches.empty()) {
            xxx::close_popup();
            return matches[std::size_t(selected_)]->id;
        }
        return std::nullopt;
    }

private:
    std::string filter_;
    int selected_ = 0;
};

// ---------------------------------------------------------------------------------------------------------
// The sidebar: an overlay at the right edge, full height, a card at the bottom
// ---------------------------------------------------------------------------------------------------------

void draw_sidebar(chat const& c, int agent) {
    auto const screen = xxx::get_screen_rect();
    xxx::overlay_begin(xxx::im_vec2(screen.max.x - sidebar_width + 1, 0), sidebar_width,
        {.border = {}, .bg = color::panel, .pad_x = 2, .pad_y = 1});
    auto rows = 0; // drawn so far, to put the card at the bottom
    auto const line = [&](std::initializer_list<im_text> parts) {
        xxx::label(parts);
        ++rows;
    };
    line({{.text = "Immediate mode chat", .attrs = xxx::im_attr_bold}});
    line({{.text = "demo session", .role = im_role::muted}});
    line({{.text = " "}});
    line({{.text = "Context", .attrs = xxx::im_attr_bold}});
    auto const tokens = std::format("{} tokens", c.tokens());
    auto const used = std::format("{}% used", c.tokens() * 100 / 200'000);
    line({{.text = tokens, .role = im_role::muted}});
    line({{.text = used, .role = im_role::muted}});
    line({{.text = " "}});
    line({{.text = "Agent", .attrs = xxx::im_attr_bold}});
    {
        auto const a =
            xxx::scoped_style(im_role::accent, {.fg = agents[std::size_t(agent)].color, .bg = {}, .attrs = 0});
        line({{.text = "● ", .role = im_role::accent}, {.text = agents[std::size_t(agent)].name}});
    }
    line({{.text = " "}});
    line({{.text = "LSP", .attrs = xxx::im_attr_bold}});
    line({{.text = "● ", .role = im_role::success}, {.text = "clangd", .role = im_role::muted}});
    line({{.text = " "}});
    line({{.text = "Modified files", .attrs = xxx::im_attr_bold}});
    line({{.text = "example/opencode.cpp  ", .role = im_role::muted}, {.text = "+412", .role = im_role::success}});
    line({{.text = "code/xxx.cpp  ", .role = im_role::muted}, {.text = "+180 ", .role = im_role::success},
        {.text = "-37", .role = im_role::error}});

    // the card: its own background across the width, inner padding by blank rows and spaces
    constexpr auto card = std::to_array<std::string_view>({"Getting started", "", "Type a message and press Enter.",
        "Tab switches the agent, Ctrl+P", "opens the commands.", ""});
    auto const available = screen.height() - 2; // without the sidebar padding
    auto const card_rows = int(card.size()) + 2;
    for (; rows < available - card_rows; ++rows) {
        xxx::label(" ");
    }
    if (rows + card_rows <= available) {
        auto const bg = xxx::scoped_style(im_role::text, {.fg = color::text, .bg = color::element, .attrs = 0});
        auto const card_line = [](im_text const& text) {
            xxx::set_next_item_width(xxx::fill());
            xxx::label({{.text = "  "}, text});
        };
        card_line({});
        card_line({.text = card[0], .attrs = xxx::im_attr_bold});
        for (auto const text : std::span(card).subspan(1)) {
            card_line({.text = text, .role = im_role::muted});
        }
    }
    xxx::overlay_end();
}

// ---------------------------------------------------------------------------------------------------------
// The status bar: activity and keys on the left, context and the palette key on the right
// ---------------------------------------------------------------------------------------------------------

void draw_status(chat const& c) {
    auto const busy = c.busy() ? activity() : std::string();
    auto const context = std::format("{:.1f}K ({}%)", c.tokens() / 1000.0, c.tokens() * 100 / 200'000);
    auto const right = std::to_array<im_text>({{.text = context, .role = im_role::muted}, {.text = "   "},
        {.text = "ctrl+p", .attrs = xxx::im_attr_bold}, {.text = " commands", .role = im_role::muted}});
    auto const right_width = width(context) + 3 + 15;
    xxx::layout_row_begin(2);
    xxx::layout_row_push(xxx::fill(right_width));
    if (c.busy()) {
        xxx::label({{.text = busy, .role = im_role::accent}, {.text = "  "},
            {.text = "esc", .attrs = xxx::im_attr_bold}, {.text = " interrupt", .role = im_role::muted}});
    } else {
        xxx::label({{.text = "tab", .attrs = xxx::im_attr_bold}, {.text = " agent   ", .role = im_role::muted},
            {.text = "pgup pgdn", .attrs = xxx::im_attr_bold}, {.text = " scroll", .role = im_role::muted}});
    }
    xxx::layout_row_push(xxx::cells(right_width));
    xxx::label(right, xxx::im_align::right);
    xxx::layout_row_end();
}

enum command_id { new_session = 1, switch_agent, toggle_sidebar, copy_answer, show_keys, quit };

constexpr auto commands = std::to_array<command>({
    {"Session", "New session", "", new_session},
    {"Session", "Copy the last answer", "", copy_answer},
    {"Agent", "Switch agent", "tab", switch_agent},
    {"View", "Toggle sidebar", "", toggle_sidebar},
    {"System", "Show all keys", "F1", show_keys},
    {"System", "Quit", "ctrl+q", quit},
});

} // namespace

int main() {
    xxx::init();
    apply_theme();

    auto conversation = chat();
    auto notes = toasts();
    auto commands_popup = palette();
    auto prompt = std::string();
    auto agent = 0;
    auto sidebar = true;
    auto scroll = 0; // rows up from the bottom
    auto prompt_focused = false;

    for (auto running = true; running;) {
        // wait for input; while answering: a tick for the next characters; a toast: until it goes away
        auto timeout = xxx::wait_forever;
        if (conversation.busy()) {
            timeout = 40ms;
        } else if (auto const t = notes.timeout(); t) {
            timeout = *t;
        }
        xxx::process_input_events(timeout);
        conversation.tick();

        xxx::new_frame();
        auto const screen = xxx::get_screen_rect();
        auto const with_sidebar = sidebar && screen.width() >= sidebar_min_screen;
        auto const popup = xxx::is_popup_open();

        if (xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
            running = false;
        }
        if (!popup) {
            if (xxx::is_key_pressed(xxx::im_key_id::ctrl_p)) {
                commands_popup.open();
            }
            if (xxx::is_key_pressed(xxx::im_key_id::tab)) {
                agent = (agent + 1) % int(agents.size());
            }
            if (xxx::is_key_pressed(xxx::im_key_id::esc)) {
                conversation.interrupt();
            }
            scroll +=
                5 * (xxx::key_press_count(xxx::im_key_id::page_up) - xxx::key_press_count(xxx::im_key_id::page_down));
        }

        // the main column: margins of one cell, the sidebar on the right
        auto const column_width = screen.width() - 2 - (with_sidebar ? sidebar_width + 1 : 0);
        xxx::layout_row_begin(2);
        xxx::layout_row_push(xxx::cells(1));
        xxx::layout_row_push(xxx::cells(column_width));

        // header: a panel with a bar
        xxx::set_next_view_look(
            {.border = xxx::im_border::none, .bg = color::panel, .pad_x = 2, .pad_y = 1, .bar_left = true});
        xxx::view_begin("##header", 0);
        xxx::label(
            {{.text = "# ", .role = im_role::muted}, {.text = "A UI like opencode", .attrs = xxx::im_attr_bold}});
        xxx::view_end();
        xxx::label(" ");

        // session: the rows that fit above the prompt, scrolled from the bottom
        constexpr auto prompt_height = 5;
        auto const session_height = std::max(1, screen.height() - 4 - prompt_height - 2);
        auto const rows = layout_rows(conversation.messages(), column_width);
        scroll = std::clamp(scroll, 0, std::max(0, int(rows.size()) - session_height));
        auto const last = int(rows.size()) - scroll;
        auto const first = std::max(0, last - session_height);
        xxx::view_begin("##session", 0, {}, session_height);
        if (rows.empty()) {
            for (int i = 0; i < session_height / 2 - 1; ++i) {
                xxx::label(" ");
            }
            xxx::label({{.text = "opencode-like UI with xxx", .role = im_role::title_focused}}, xxx::im_align::center);
            xxx::label({{.text = "type a message below", .role = im_role::muted}}, xxx::im_align::center);
        }
        for (int i = first; i < last; ++i) {
            draw_row(rows[std::size_t(i)]);
        }
        xxx::view_end();

        // prompt: a lighter panel, the bar in the color of the agent
        {
            auto const bar = xxx::scoped_style(
                im_role::border_focused, {.fg = agents[std::size_t(agent)].color, .bg = {}, .attrs = 0});
            xxx::set_next_view_look(
                {.border = xxx::im_border::none, .bg = color::element, .pad_x = 2, .pad_y = 1, .bar_left = true});
            xxx::view_begin("##prompt", 0);
            if (!prompt_focused) {
                xxx::set_focus_next(); // the prompt keeps the keyboard, also after a click elsewhere
            }
            xxx::set_next_item_width(xxx::fill());
            auto const sent =
                xxx::text_input("Ask anything...  \"Explain how overlays work\"", prompt, xxx::im_input_flag_no_prompt);
            prompt_focused = xxx::is_item_focused();
            if (sent && !prompt.empty() && !conversation.busy()) {
                conversation.send(std::exchange(prompt, {}), agent);
                scroll = 0;
            }
            xxx::label(" ");
            auto const a =
                xxx::scoped_style(im_role::accent, {.fg = agents[std::size_t(agent)].color, .bg = {}, .attrs = 0});
            xxx::label({{.text = agents[std::size_t(agent)].name, .role = im_role::accent, .attrs = xxx::im_attr_bold},
                {.text = "  demo-model", .role = im_role::text}, {.text = " · offline", .role = im_role::muted}});
            xxx::view_end();
        }
        xxx::label(" ");
        draw_status(conversation);
        xxx::layout_row_end();

        if (with_sidebar) {
            draw_sidebar(conversation, agent);
        }
        notes.draw(column_width); // the right edge of the main column

        xxx::key_hint("Enter", "send");
        xxx::key_hint("Tab", "switch agent");
        xxx::key_hint("c-p", "commands");
        xxx::key_hint("Esc", "interrupt the answer");
        xxx::key_hint("c-q", "quit");

        if (auto const chosen = commands_popup.draw(commands); chosen) {
            switch (*chosen) {
            case new_session:
                conversation.clear();
                notes.push("new session started");
                break;
            case switch_agent: agent = (agent + 1) % int(agents.size()); break;
            case toggle_sidebar: sidebar = !sidebar; break;
            case copy_answer: {
                auto text = std::string();
                for (auto const& m : conversation.messages()) {
                    if (m.type == message::kind::user) {
                        text.clear();
                    } else if (m.type == message::kind::assistant) {
                        text += m.text + "\n";
                    }
                }
                if (text.empty()) {
                    notes.push("nothing to copy yet", true);
                } else {
                    xxx::set_clipboard(text);
                    notes.push("copied to the clipboard");
                }
            } break;
            case show_keys: xxx::open_popup("##xxx-help"); break; // the built-in help, same as F1
            case quit: running = false; break;
            default: break;
            }
        }
        xxx::render();
    }

    xxx::shutdown();
    return 0;
}
