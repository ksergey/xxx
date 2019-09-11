// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#include <iostream>
#include <thread>

#include <termbox.h>
#include <xxx/ui.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    bool running = true;

    float spinner_step_storage = 0.0;
    float progress_value = 33.3;
    std::string text;
    xxx::text_input_context text_input;

    xxx::init();

    while (running) {
      xxx::update(75);

      if (xxx::is_key_pressed(xxx::key::esc)) {
        running = false;
      }
      if (xxx::is_key_pressed(xxx::key::arrow_left)) {
        progress_value -= 0.6;
      }
      if (xxx::is_key_pressed(xxx::key::arrow_right)) {
        progress_value += 0.6;
      }

      xxx::begin();

      // clang-format off
      xxx::row_begin(2);
        xxx::row_push(0.4);
          xxx::panel_begin("PANEL 1.A");
            xxx::text("Content -3", xxx::make_color(255, 0, 0));
            xxx::text("Content -2", xxx::make_color(255, 61, 127));
            xxx::text("Content -1", xxx::make_color(255, 158, 157));
            xxx::text("Content 0", xxx::color::default_);
          xxx::panel_end();
          xxx::panel_begin("PANEL 1.B");
            using namespace xxx::literals;
            xxx::text("Content a", 0xFF6B6B_c);
            xxx::text("Content b", 0xC7F464_c);
            xxx::text("Content c0", 0x556270_c);
            xxx::text("Content c1", xxx::make_color(85, 98, 112));
          xxx::panel_end();
          xxx::panel_begin("PROGRESS");
            xxx::progress(progress_value);
          xxx::panel_end();
          xxx::panel_begin("INPUT");
            if (xxx::text_input(text_input)) {
              xxx::text_input_clear(text_input);
            }
          xxx::panel_end();
        xxx::row_push(0.6);
          xxx::panel_begin("PANEL 2");
            xxx::text("Content 1");
            xxx::spacer(0.2);
          xxx::panel_end();
          xxx::panel_begin("PANEL 3");
            xxx::text("Content 2");
          xxx::panel_end();
          xxx::panel_begin("PANEL 4");
            xxx::spinner(spinner_step_storage, "Loading", xxx::align::center);
          xxx::panel_end();
      xxx::row_end();
      // clang-format on

      xxx::end();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::cout << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
