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

    xxx::init();

    xxx::set_key_event_handler([&running, &progress_value](xxx::key const& key) {
      if (key.key == 27 || key.ch == 'q') {
        running = false;
      } else if (key.ch == '1') {
        progress_value -= 0.66;
      } else if (key.ch == '2') {
        progress_value += 0.66;
      }
    });

    while (running) {
      xxx::update();

      xxx::begin();

      // clang-format off
      xxx::row_begin(2);
        xxx::row_push(0.4);
          xxx::panel_begin("Panel 1.a");
            xxx::text("Content -3", xxx::make_color(255, 0, 0));
            xxx::text("Content -2", xxx::make_color(255, 61, 127));
            xxx::text("Content -1", xxx::make_color(255, 158, 157));
            xxx::text("Content 0", xxx::color::default_);
          xxx::panel_end();
          xxx::panel_begin("Panel 1.b");
            using namespace xxx::literals;
            xxx::text("Content a", 0xFF6B6B_c);
            xxx::text("Content b", 0xC7F464_c);
            xxx::text("Content c0", 0x556270_c);
            xxx::text("Content c1", xxx::make_color(85, 98, 112));
          xxx::panel_end();
          xxx::panel_begin("Progress");
            xxx::progress(progress_value);
          xxx::panel_end();
          xxx::panel_begin("Input");
            if (xxx::text_input(text)) {
              // Input.
            }
          xxx::panel_end();
        xxx::row_push(0.6);
          xxx::panel_begin("Panel 2");
            xxx::text("Content 1");
            xxx::spacer(0.2);
          xxx::panel_end();
          xxx::panel_begin("Panel 3");
            xxx::text("Content 2");
          xxx::panel_end();
          xxx::panel_begin("Panel 4");
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
