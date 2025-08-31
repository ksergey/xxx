// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <print>

#include <xxx2.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    xxx::init();

    bool done = false;

    while (!done) {
      xxx::poll_events(std::chrono::milliseconds(25));

      xxx::new_frame();

      xxx::label("🔑 hello 🔒");

      xxx::render();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::print(stderr, "ERROR: {}\n", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
