// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#include <iostream>

#include <xxx/ui.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    bool running = true;

    float spinner_step_storage = 0.0;
    float progress_value = 33.3;
    bool show_text_enter{false};
    std::string text;

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
      if (!show_text_enter) {
        show_text_enter = xxx::is_key_pressed(xxx::key::enter);
      }

      xxx::begin();

      // clang-format off
      xxx::row_begin(2);
        xxx::row_push(0.4);
          xxx::panel_begin("PANEL 1.A");
            xxx::label("Content -3", xxx::make_color(255, 0, 0));
            xxx::label("Content -2", xxx::make_color(255, 61, 127));
            xxx::label("Content -1", xxx::make_color(255, 158, 157));
            xxx::label("Content 0", xxx::color::default_);
          xxx::panel_end();
          xxx::panel_begin("PANEL 1.B");
            using namespace xxx::literals;
            xxx::label("Content a", 0xFF6B6B_c);
            xxx::label("Content b", 0xC7F464_c);
            xxx::label("Content c0", 0x556270_c);
            xxx::label("Content c1", xxx::make_color(85, 98, 112));
          xxx::panel_end();
          xxx::panel_begin("PROGRESS");
            xxx::progress(progress_value);
          xxx::panel_end();
        xxx::row_push(0.6);
          xxx::panel_begin("PANEL 2");
            xxx::label("Content 1");
            xxx::spacer(0.2);
          xxx::panel_end();
          xxx::panel_begin("PANEL 3");
            xxx::label("Content 2");
          xxx::panel_end();
          xxx::panel_begin("PANEL 4");
            xxx::spinner(spinner_step_storage, "Loading", xxx::align::center);
          xxx::panel_end();
      xxx::row_end();

      if (show_text_enter) {
        xxx::fixed_panel_begin({10, 5, 20, 20});
          xxx::label("Enter a message:");
          if (xxx::text_input(text)) {
            text.clear();
            show_text_enter = false;
          }
        xxx::fixed_panel_end();
      }
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
