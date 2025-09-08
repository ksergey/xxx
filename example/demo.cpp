// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <print>

#include <xxx2.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    xxx::init();

    bool done = false;

    while (!done) {
      xxx::poll_terminal_events(std::chrono::milliseconds(25));

      if (xxx::is_key_pressed(xxx::im_key_id::quit)) {
        break;
      }

      xxx::new_frame();

      xxx::debug();
      xxx::label("🔑 hello 🔒");
      xxx::label("");

      auto bounds = xxx::layout_get_space_bounds();
      xxx::label("space bound: (({}, {}), ({}, {}))", bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y);
      xxx::debug_xxx();
      xxx::debug_rect();
      xxx::render();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::print(stderr, "ERROR: {}\n", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
