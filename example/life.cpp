// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// Conway's Game of Life on a braille canvas: every terminal cell shows 2x4 cells of the world.
//
// Keys: F1 or ? shows them all, the bottom line shows keys of the focused widget.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <format>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <xxx.h>

namespace {

using namespace std::string_view_literals;
using namespace xxx::literals;

struct pattern {
    std::string_view name;
    std::string_view kind;
    std::array<std::string_view, 13> rows; // 'O' alive, anything else dead; empty rows ignored
};

// clang-format off
constexpr auto patterns = std::to_array<pattern>({
  {"glider", "ship", {".O.", "..O", "OOO"}},
  {"spaceship", "ship", {".O..O", "O....", "O...O", "OOOO."}},
  {"r-pentomino", "chaos", {".OO", "OO.", ".O."}},
  {"acorn", "chaos", {".O.....", "...O...", "OO..OOO"}},
  {"diehard", "vanish", {"......O.", "OO......", ".O...OOO"}},
  {"pulsar", "osc", {
    "..OOO...OOO..", ".............", "O....O.O....O", "O....O.O....O", "O....O.O....O",
    "..OOO...OOO..", ".............", "..OOO...OOO..", "O....O.O....O", "O....O.O....O",
    "O....O.O....O", ".............", "..OOO...OOO.."}},
  {"glider gun", "gun", {
    "........................O...........",
    "......................O.O...........",
    "............OO......OO............OO",
    "...........O...O....OO............OO",
    "OO........O.....O...OO..............",
    "OO........O...O.OO....O.O...........",
    "..........O.....O.......O...........",
    "...........O...O....................",
    "............OO......................"}},
});
// clang-format on

constexpr auto speeds = std::to_array({1, 2, 5, 10, 20, 30, 60}); // generations per second

constexpr auto pattern_columns = std::to_array<xxx::im_table_column>({
    {"pattern", xxx::fill()},
    {"cells", xxx::cells(5), xxx::im_align::right},
    {"kind", xxx::cells(7), xxx::im_align::right},
});

// row-major cells for the patterns table
[[nodiscard]] auto pattern_cells() -> std::vector<std::string> {
    auto cells = std::vector<std::string>();
    for (auto const& p : patterns) {
        auto alive = 0;
        for (auto const row : p.rows) {
            alive += int(std::count(row.begin(), row.end(), 'O'));
        }
        cells.emplace_back(p.name);
        cells.push_back(std::to_string(alive));
        cells.emplace_back(p.kind);
    }
    return cells;
}

constexpr auto info_tabs = std::to_array({"events"sv, "rules"sv});

constexpr auto rules_text = std::to_array({
    "every cell has 8 neighbours"sv,
    ""sv,
    "dead cell with exactly 3"sv,
    "  live neighbours is born"sv,
    "live cell with 2 or 3"sv,
    "  live neighbours survives"sv,
    "all other cells die"sv,
    ""sv,
    "color shows cell age:"sv,
    "  newborn -> old-timer"sv,
});

class world {
public:
    void resize(int width, int height) {
        if (width == width_ && height == height_) {
            return;
        }
        // keep overlapping part, so resizing terminal doesn't wipe the world
        auto next = std::vector<std::uint16_t>(std::size_t(width * height));
        for (int y = 0; y < std::min(height, height_); ++y) {
            for (int x = 0; x < std::min(width, width_); ++x) {
                next[std::size_t(y * width + x)] = at(x, y);
            }
        }
        width_ = width;
        height_ = height;
        cells_ = std::move(next);
        scratch_.resize(cells_.size());
    }

    void randomize(std::mt19937& rng, double density = 0.25) {
        auto dist = std::bernoulli_distribution(density);
        for (auto& c : cells_) {
            c = dist(rng) ? 1 : 0;
        }
    }

    void clear() {
        std::fill(cells_.begin(), cells_.end(), 0);
    }

    void stamp(pattern const& p, int x0, int y0) {
        for (int y = 0; auto const row : p.rows) {
            for (int x = 0; x < int(row.size()); ++x) {
                set(x0 + x, y0 + y, row[std::size_t(x)] == 'O');
            }
            ++y;
        }
    }

    void step(bool wrap) {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                auto n = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if ((dx || dy) && alive(x + dx, y + dy, wrap)) {
                            ++n;
                        }
                    }
                }
                auto const age = at(x, y);
                auto& next = scratch_[std::size_t(y * width_ + x)];
                if (age > 0) {
                    next = (n == 2 || n == 3) ? std::uint16_t(std::min(age + 1, 0xffff)) : 0;
                } else {
                    next = (n == 3) ? 1 : 0;
                }
            }
        }
        cells_.swap(scratch_);
        ++generation_;
    }

    [[nodiscard]] auto at(int x, int y) const -> std::uint16_t {
        return cells_[std::size_t(y * width_ + x)];
    }
    [[nodiscard]] auto width() const noexcept -> int {
        return width_;
    }
    [[nodiscard]] auto height() const noexcept -> int {
        return height_;
    }
    [[nodiscard]] auto generation() const noexcept -> long {
        return generation_;
    }
    [[nodiscard]] auto population() const -> int {
        return int(std::count_if(cells_.begin(), cells_.end(), [](auto c) {
            return c > 0;
        }));
    }
    void reset_generation() noexcept {
        generation_ = 0;
    }

private:
    [[nodiscard]] auto alive(int x, int y, bool wrap) const -> bool {
        if (wrap) {
            x = (x + width_) % width_;
            y = (y + height_) % height_;
        } else if (x < 0 || y < 0 || x >= width_ || y >= height_) {
            return false;
        }
        return at(x, y) > 0;
    }
    void set(int x, int y, bool value) {
        x = (x % width_ + width_) % width_;
        y = (y % height_ + height_) % height_;
        cells_[std::size_t(y * width_ + x)] = value ? 1 : 0;
    }

    int width_ = 0;
    int height_ = 0;
    long generation_ = 0;
    std::vector<std::uint16_t> cells_;
    std::vector<std::uint16_t> scratch_;
};

// newborns are hot, old-timers cool down
// colors of the terminal scheme: readable on light and dark backgrounds
[[nodiscard]] auto age_color(std::uint16_t age) -> xxx::im_color {
    if (age <= 1) {
        return xxx::ansi::bright_yellow;
    }
    if (age <= 3) {
        return xxx::ansi::bright_green;
    }
    if (age <= 10) {
        return xxx::ansi::cyan;
    }
    if (age <= 40) {
        return xxx::ansi::blue;
    }
    return xxx::ansi::magenta;
}

constexpr int sidebar_width = 28;

} // namespace

int main() {
    xxx::init();

    auto rng = std::mt19937(std::random_device()());
    auto life = world();
    auto running = true;
    auto wrap = true;
    auto colorful = true;
    auto speed_index = 3;
    auto selected_pattern = 0;
    auto history = std::deque<int>();
    auto events = std::deque<std::string>();
    auto spinner_step = 0.0f;
    auto info_tab = 0;
    auto focus_keep = false;
    auto const patterns_table = pattern_cells();
    auto quit = false;

    auto const note = [&](std::string message) {
        events.push_front(std::format("{:>5} {}", life.generation(), message));
        if (events.size() > 100) {
            events.pop_back();
        }
    };

    auto last_step = std::chrono::steady_clock::now();
    auto first_frame = true;

    while (!quit) {
        // event driven: sleep until input or until the next generation is due.
        // Paused world costs no CPU; running one draws a frame per generation
        // (at least every 100 ms to keep the spinner moving) instead of 60 per second.
        auto timeout = xxx::wait_forever;
        if (first_frame) {
            timeout = std::chrono::milliseconds(0);
        } else if (running) {
            auto const period = std::chrono::milliseconds(1000 / speeds[std::size_t(speed_index)]);
            auto const due = last_step + period - std::chrono::steady_clock::now();
            timeout = std::clamp(std::chrono::duration_cast<std::chrono::milliseconds>(due),
                std::chrono::milliseconds(0), std::chrono::milliseconds(100));
        }
        xxx::process_input_events(timeout);
        if (xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
            break;
        }

        // world takes the right part of the screen, inside a border
        auto const screen = xxx::get_screen_rect();
        // border around the world and the key hints line below
        auto const canvas_cells =
            xxx::im_vec2(std::max(1, screen.width() - sidebar_width - 2), std::max(1, screen.height() - 3));
        life.resize(canvas_cells.x * 2, canvas_cells.y * 4);
        if (first_frame) {
            life.randomize(rng);
            note(std::format("hello, {}x{} world", life.width(), life.height()));
            first_frame = false;
        }

        // simulation runs at its own pace, independent of frame rate
        auto const now = std::chrono::steady_clock::now();
        auto const period = std::chrono::milliseconds(1000 / speeds[std::size_t(speed_index)]);
        if (running && now - last_step >= period) {
            life.step(wrap);
            last_step = now;
            history.push_back(life.population());
            if (history.size() > 2 * (sidebar_width - 2)) {
                history.pop_front();
            }
        }

        xxx::new_frame();

        xxx::layout_row_begin(2);
        xxx::layout_row_push(xxx::cells(sidebar_width));
        {
            xxx::view_begin("controls", xxx::im_key_id::ctrl_o);
            if (xxx::button(running ? "pause##run" : "play##run")) {
                running = !running;
                note(running ? "play" : "pause");
            }
            xxx::same_line();
            if (xxx::button("step")) {
                life.step(wrap);
                running = false;
            }
            if (xxx::button("random")) {
                life.randomize(rng);
                life.reset_generation();
                history.clear();
                note("random soup");
            }
            xxx::same_line();
            if (xxx::button("clear")) {
                xxx::open_popup("clear");
                focus_keep = true; // safe default: Enter must not wipe the world
            }
            if (xxx::button("slower") && speed_index > 0) {
                --speed_index;
            }
            xxx::same_line();
            if (xxx::button("faster") && speed_index + 1 < int(speeds.size())) {
                ++speed_index;
            }
            xxx::label(std::format("speed: {} gen/s", speeds[std::size_t(speed_index)]));
            if (xxx::checkbox("wrap edges", wrap)) {
                note(wrap ? "torus world" : "flat world");
            }
            xxx::checkbox("color by age", colorful);
            xxx::view_end();

            xxx::view_begin("patterns", xxx::im_key_id::ctrl_p);
            if (xxx::table("patterns", pattern_columns, patterns_table, selected_pattern, 4)) {
                auto const& p = patterns[std::size_t(selected_pattern)];
                auto const x = std::uniform_int_distribution(0, life.width() - 1)(rng);
                auto const y = std::uniform_int_distribution(0, life.height() - 1)(rng);
                life.stamp(p, x, y);
                note(std::format("{} at {},{}", p.name, x, y));
            }
            xxx::view_end();

            xxx::view_begin("stats");
            auto const population = life.population();
            xxx::label(std::format("generation {}", life.generation()));
            xxx::label(std::format("population {}", population));
            if (running) {
                xxx::spinner("evolving", spinner_step);
            } else {
                xxx::label("paused");
            }
            // population history: one column of braille dots per generation
            if (xxx::canvas_begin(xxx::im_vec2(2 * (sidebar_width - 2), 8))) {
                auto const peak = history.empty() ? 1 : std::max(1, *std::max_element(history.begin(), history.end()));
                for (int x = 0; auto const value : history) {
                    auto const h = std::clamp(value * 8 / peak, 1, 8);
                    for (int y = 8 - h; y < 8; ++y) {
                        xxx::canvas_point(xxx::im_vec2(x, y), xxx::ansi::blue);
                    }
                    ++x;
                }
                xxx::canvas_end();
            }
            xxx::view_end();

            xxx::view_begin("info", xxx::im_view_flags_default, xxx::im_key_id::ctrl_e, xxx::fill(1));
            xxx::tabs("info", info_tabs, info_tab);
            switch (info_tab) {
            case 0:
                for (auto const& e : events) {
                    xxx::label(e);
                }
                break;
            default:
                for (auto const line : rules_text) {
                    xxx::label(line.empty() ? " "sv : line);
                }
                break;
            }
            xxx::view_end();
        }
        xxx::layout_row_push(xxx::fill());
        {
            xxx::view_begin(std::format("life{}##world", running ? "" : " (paused)"), xxx::im_view_flags_default,
                xxx::im_key_id(), xxx::fill(1));
            if (xxx::canvas_begin(xxx::im_vec2(life.width(), life.height()))) {
                for (int y = 0; y < life.height(); ++y) {
                    for (int x = 0; x < life.width(); ++x) {
                        if (auto const age = life.at(x, y); age > 0) {
                            xxx::canvas_point(xxx::im_vec2(x, y), colorful ? age_color(age) : xxx::im_color());
                        }
                    }
                }
                xxx::canvas_end();
            }
            xxx::view_end();
        }
        xxx::layout_row_end();
        xxx::key_hint("c-q", "quit");
        xxx::key_hint("Enter", "on a pattern: stamp it at a random place");
        xxx::key_hints("c-q quit");

        if (xxx::popup_begin("clear", "clear the world?", 34)) {
            xxx::label(std::format("{} cells after {} generations", life.population(), life.generation()));
            xxx::label("will be gone forever");
            xxx::label(" ");
            if (xxx::button("clear")) {
                life.clear();
                life.reset_generation();
                history.clear();
                note("cleared");
                xxx::close_popup();
            }
            xxx::same_line();
            if (std::exchange(focus_keep, false)) {
                xxx::set_focus_next();
            }
            if (xxx::button("keep")) {
                xxx::close_popup();
            }
            xxx::popup_end();
        }

        xxx::render();
    }

    xxx::shutdown();
    return 0;
}
