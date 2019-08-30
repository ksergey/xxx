#include <iostream>
#include <thread>

#include <termbox.h>
#include <xxx/ui.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    bool running = true;

    float spinner_step_storage = 0.0;

    xxx::init();

    xxx::set_key_event_handler([&running](xxx::key const& key) {
      if (key.key == 27 || key.ch == 'q') {
        running = false;
      }
    });

    while (running) {
      xxx::update();

      xxx::begin();

      // clang-format off
      xxx::row_begin(2);
        xxx::row_push(0.4);
          xxx::panel_begin("Panel 1");
            xxx::text("Content -3", xxx::color::red);
            xxx::text("Content -2", xxx::color::cyan);
            xxx::text("Content -1", xxx::color::magenta);
            xxx::text("Content 0");
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
            xxx::spinner(spinner_step_storage, "Content loading");
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
