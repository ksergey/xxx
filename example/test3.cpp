// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <cstdio>
#include <exception>
#include <generator>
#include <print>
#include <tuple>

#include <xxx-internal.h>
#include <xxx-unicode.h>

[[nodiscard]] auto align_gen() -> std::generator<std::tuple<xxx::im_halign, xxx::im_valign>> {
  while (true) {
    for (auto valign : {xxx::im_valign::top, xxx::im_valign::center, xxx::im_valign::bottom}) {
      for (auto halign : {xxx::im_halign::left, xxx::im_halign::center, xxx::im_halign::right}) {
        co_yield {halign, valign};
      }
    }
  }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    xxx::init();

    auto& renderer = xxx::get_context()->renderer;

    for (auto [halign, valign] : align_gen()) {
      xxx::update(std::chrono::milliseconds(1000) / 5);

      if (xxx::is_key_pressed(xxx::im_key_id::quit)) {
        break;
      }

      xxx::new_frame();

      renderer.fill_rect(xxx::im_rect(1, 1, 20, 10), '.', xxx::make_color(1.0, 0.0, 0.0));
      renderer.fill_rect(xxx::im_rect(15, 5, 35, 15), '.', xxx::make_color(0.0, 1.0, 0.0));
      renderer.draw_rect(xxx::im_rect(20, 10, 40, 20), xxx::make_color(0.3, 0.9, 1.0));

      // renderer.set_clip_rect(xxx::im_rect(18, 6, 25, 9));
      renderer.draw_text_at(
          xxx::im_vec2(16, 7), xxx::utf8_to_unicode("hello world sweet orange"), xxx::make_color(1.0, 0.3, 0.9));

      // renderer.set_clip_rect(xxx::im_rect(0, 15, 30, 25).crop(1, 1, 1, 1));
      renderer.push_clip_rect(xxx::im_rect(0, 15, 30, 25).crop_right(1));

      renderer.draw_text_in_rect(xxx::im_rect(0, 15, 30, 25), xxx::utf8_to_unicode("test :)"),
          xxx::im_style(xxx::make_color(0.8f, 0.7f, 1.0f), xxx::make_color(0.2f, 0.4f, 0.3f)), halign, valign);

      xxx::render();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::print(stderr, "ERROR: {}\n", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
