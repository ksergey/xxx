// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <cmath>
#include <iostream>
#include <numbers>

#include <xxx.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    bool running = true;

    float progressValue = 33.3;
    bool showTextInput = false;
    std::string text;

    xxx::init();

    while (running) {
      xxx::update();

      if (auto const lastInput = xxx::lastInputEvent(); lastInput) {
        if (lastInput->key == TB_KEY_ESC) {
          running = false;
        } else if (lastInput->key == TB_KEY_ARROW_LEFT) {
          progressValue -= 0.6;
        } else if (lastInput->key == TB_KEY_ARROW_RIGHT) {
          progressValue += 0.6;
        }
        if (!showTextInput) {
          showTextInput = lastInput->key == TB_KEY_ENTER;
        }
      }

      xxx::begin();

      // clang-format off
      xxx::rowBegin(2);
        xxx::rowPush(0.4);
          xxx::panelBegin();
          xxx::panelTitle("PANEL 1.A");
            xxx::label("Content -3");
            xxx::label("Content -2");
            xxx::label("Content -1");
            xxx::label("Content 0");
          xxx::panelEnd();
          xxx::panelBegin();
          xxx::panelTitle("PANEL 1.B");
            xxx::label("Content a" );
            xxx::label("Content b");
            xxx::label("Content c0");
            xxx::label("Content c1");
          xxx::panelEnd();
          xxx::panelBegin();
          xxx::panelTitle("PROGRESS");
            xxx::progress(progressValue);
          xxx::panelEnd();
        xxx::rowPush(0.6);
          xxx::panelBegin();
          xxx::panelTitle("PANEL 2");
            xxx::stylePush(xxx::rgb(33, 33, 255));
            xxx::label("Content 1");
            xxx::stylePop();
            xxx::spacer(0.2);
          xxx::panelEnd();
          xxx::panelBegin();
          xxx::panelTitle("PANEL 3");
            xxx::label("Content 2");
          xxx::panelEnd();
          xxx::stylePush(xxx::rgb(255, 0, 0));
          xxx::panelBegin();
          xxx::panelTitle("PANEL 4");
          xxx::stylePop();
            xxx::spinner("Loading", xxx::Alignment::Center);
          xxx::panelEnd();
      xxx::rowEnd();
      // clang-format on

      // xxx::canvas(0.99, [&](xxx::Canvas& c) {
      //   auto const drawCircle = [&](int x0, int y0, int r, xxx::Color color) {
      //     for (float angle = 0.0; angle < 360.0; angle += 0.1) {
      //       float const arg = angle * std::numbers::pi_v<float> / 180.0;
      //       int const x = r * std::cos(arg);
      //       int const y = r * std::sin(arg);
      //       c.point(x0 + x, y0 + y, color);
      //     }
      //   };
      //   drawCircle(c.width() / 2, c.height() / 2, c.height() / 2 - 1, xxx::color(33, 255, 128));
      //   drawCircle(c.width() / 3, c.height() / 3, c.height() / 4 - 1, xxx::color(64, 128, 255));
      //   drawCircle(c.width() * 2 / 3, c.height() / 3, c.height() / 5, xxx::color(255, 128, 255));
      //   drawCircle(c.width() / 3, c.height() * 2 / 3, c.height() / 3.5, xxx::color(64, 64, 255));
      // });

      xxx::end();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::cout << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
