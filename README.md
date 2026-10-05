# xxx

Immediate mode text ui library for C++23. No dependencies: it talks to the terminal directly.
Earlier versions were built on [termbox/termbox2](https://github.com/termbox/termbox2), thanks to its authors.

![Game of Life example](docs/life.svg)

## Features

- Immediate mode API: describe UI every frame, no widget objects to manage
- Event driven: frames are built on input, timeout or `wake_up()` from another thread; idle app uses no CPU
- Automatic layout: rows of `cells(n)`, `ratio(r)` or `fill()` wide columns, `same_line`
- Views with border, title and shortcut; fixed, fit-content or `fill()` height
- Scrolling: PgUp / PgDn, mouse wheel, auto-scroll to focused widget
- Widgets: label, button, checkbox, list, table, tabs, text input (password, readline keys), spinner, progress
- Clipboard via OSC 52 (works over ssh and in tmux): ctrl-c in inputs, lists and tables; readline kill / yank
- Modal popups drawn on top of everything, with input capture and focus restore
- Keyboard focus: arrows move to the nearest widget (also across views), Tab / Shift-Tab, `set_focus` and mouse (click to focus, press, place cursor)
- Id scopes (`push_id` / `pop_id`) for widgets built in loops
- Unicode aware: wide chars (CJK, emoji) take two cells
- Braille canvas: 2x4 "pixels" per cell
- Color theming
- Headless backend for testing UI without a terminal
- Debug builds mark widgets with colliding ids (same label twice) with a red `!`

## Example

```cpp
#include <xxx.h>

int main() {
  xxx::init();
  auto name = std::string();
  auto agree = false;
  while (true) {
    // sleep until input arrives: no CPU spent while nothing happens
    xxx::process_input_events(xxx::wait_forever);
    if (xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
      break;
    }
    xxx::new_frame();
    xxx::view_begin("hello");
    xxx::text_input("your name", name);
    xxx::checkbox("I like terminals", agree);
    if (xxx::button("greet")) {
      // ...
    }
    xxx::view_end();
    xxx::render();
  }
  xxx::shutdown();
}
```

[`example/life.cpp`](example/life.cpp) is Conway's Game of Life (screenshot above):
play / pause, speed, patterns table, population chart, event log and help in tabs, confirmation popup.

```sh
cmake -B build && cmake --build build
./build/example/life
```

[`example/mihomo.cpp`](example/mihomo.cpp) is a dashboard for [mihomo](https://github.com/MetaCubeX/mihomo)
(Clash.Meta) through its RESTful API over http or https: traffic, proxy groups (select a proxy, test
delays) and connections (close them). It shows how to combine the UI with network: background threads
do the requests and call `xxx::wake_up()` when new data arrives, the UI thread sleeps in
`process_input_events(wait_forever)` and never blocks on the network.

```sh
# needs libcurl (e.g. apt install libcurl4-openssl-dev); nlohmann_json and cxxopts come with CPM
./build/example/mihomo -u https://127.0.0.1:9090 -s "$SECRET" --cacert controller.crt
```

## Terminal support

The library talks to the terminal directly (no curses, no terminfo) and works with xterm compatible
terminals: practically every modern one (xterm, GNOME Terminal, Konsole, iTerm2, kitty, WezTerm,
Alacritty, Windows Terminal over ssh, tmux, screen). It uses bracketed paste (pasted newlines don't
press Enter), synchronized output (no flicker) and restores the terminal even when the app is killed
by a signal or crashes.

Environment:

- `ESCDELAY=ms`: how long a lone Esc waits for the rest of a key sequence (default 25, raise it for slow ssh links)

## Testing

UI is tested with the headless backend: build a frame, compare the screen.

```cpp
TEST_CASE("button") {
  headless_app app(im_vec2(20, 3));
  app.frame([] {
    view_begin("main");
    button("ok");
    view_end();
  });
  CHECK(app.screen() == dedent(R"(
    ╭────── main ──────╮
    │  [ ok ]          │
    ╰──────────────────╯
  )"));
}
```

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```
