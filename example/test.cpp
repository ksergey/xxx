// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <cstdio>
#include <exception>
#include <print>

#include <xxx.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    xxx::init();

    bool done = false;

    while (!done) {
      xxx::update(std::chrono::milliseconds(25));

      if (xxx::is_key_pressed(xxx::im_key_id::quit)) {
        break;
      }

      xxx::new_frame();
      xxx::debug();
      xxx::render();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::print(stderr, "ERROR: {}\n", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
