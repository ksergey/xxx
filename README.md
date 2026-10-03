# xxx

Immediate mode text ui library for C++23.
Built on top of [termbox/termbox2](https://github.com/termbox/termbox2) (previously [nsf/termbox](https://github.com/nsf/termbox)).

![Game of Life example](docs/life.svg)

## Features

- Immediate mode API: describe UI every frame, no widget objects to manage
- Automatic layout: rows with ratio, fixed or `fill()` width columns, `same_line`
- Views with border, title and shortcut; fixed, fit-content or `fill()` height
- Scrolling: PgUp / PgDn, mouse wheel, auto-scroll to focused widget
- Widgets: label, button, checkbox, list, table, tabs, text input (password, readline keys), spinner, progress
- Modal popups drawn on top of everything, with input capture and focus restore
- Keyboard focus (Tab / Shift-Tab, `set_focus`) and mouse (click to focus, press, place cursor)
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
    xxx::process_input_events();
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
