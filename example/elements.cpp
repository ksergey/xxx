// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#include <iostream>

#include <xxx.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    bool running = true;

    float progressValue = 33.3;
    bool showTextEnter = false;
    std::string text;

    xxx::init();

    while (running) {
      xxx::update(75);

      if (xxx::isKeyPressed(xxx::key::Esc)) {
        running = false;
      }
      if (xxx::isKeyPressed(xxx::key::ArrowLeft)) {
        progressValue -= 0.6;
      }
      if (xxx::isKeyPressed(xxx::key::ArrowRight)) {
        progressValue += 0.6;
      }
      if (!showTextEnter) {
        showTextEnter = xxx::isKeyPressed(xxx::key::Enter);
      }

      xxx::begin();

      // clang-format off
      xxx::rowBegin(2);
        xxx::rowPush(0.4);
          xxx::panelBegin("PANEL 1.A");
            xxx::label("Content -3");
            xxx::label("Content -2");
            xxx::label("Content -1");
            xxx::label("Content 0");
          xxx::panelEnd();
          xxx::panelBegin("PANEL 1.B");
            using namespace xxx::literals;
            xxx::label("Content a" );
            xxx::label("Content b");
            xxx::label("Content c0");
            xxx::label("Content c1");
          xxx::panelEnd();
          xxx::panelBegin("PROGRESS");
            xxx::progress(progressValue);
          xxx::panelEnd();
        xxx::rowPush(0.6);
          xxx::panelBegin("PANEL 2");
            xxx::label("Content 1");
            xxx::spacer(0.2);
          xxx::panelEnd();
          xxx::panelBegin("PANEL 3");
            xxx::label("Content 2");
          xxx::panelEnd();
          xxx::panelBegin("PANEL 4");
            xxx::spinner("Loading", xxx::Align::Center);
          xxx::panelEnd();
      xxx::rowEnd();
      // clang-format on

      xxx::canvas(0.99, [&](xxx::Canvas& c) {
        for (int i = 0; i < c.width(); i++) {
          c.point(i, c.height() / 2, xxx::color(66, 128, 255));
        }
        for (int i = 0; i < c.width(); i++) {
          c.point(i, c.height() - 1, xxx::color(255, 128, 33));
        }
        for (int i = 0; i < c.width(); i++) {
          c.point(i, 0, xxx::color(33, 255, 128));
        }
        for (int i = 0; i < c.height(); i++) {
          c.point(c.width() / 3, i, xxx::color(66, 66, 255));
        }
      });

      xxx::end();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::cout << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
