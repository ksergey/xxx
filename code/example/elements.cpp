// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#include <iostream>

#include <xxx/ui.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    bool running = true;

    float spinnerStepStorage = 0.0;
    float progressValue = 33.3;
    bool showTextEnter = false;
    std::string text;

    xxx::init();

    while (running) {
      xxx::update(75);

      if (xxx::isKeyPressed(xxx::Key::Esc)) {
        running = false;
      }
      if (xxx::isKeyPressed(xxx::Key::ArrowLeft)) {
        progressValue -= 0.6;
      }
      if (xxx::isKeyPressed(xxx::Key::ArrowRight)) {
        progressValue += 0.6;
      }
      if (!showTextEnter) {
        showTextEnter = xxx::isKeyPressed(xxx::Key::Enter);
      }

      xxx::begin();

      // clang-format off
      xxx::rowBegin(2);
        xxx::rowPush(0.4);
          xxx::panelBegin("PANEL 1.A");
            xxx::label("Content -3", xxx::makeColor(255, 0, 0));
            xxx::label("Content -2", xxx::makeColor(255, 61, 127));
            xxx::label("Content -1", xxx::makeColor(255, 158, 157));
            xxx::label("Content 0", xxx::Color::Default);
          xxx::panelEnd();
          xxx::panelBegin("PANEL 1.B");
            using namespace xxx::literals;
            xxx::label("Content a", 0xFF6B6B_c);
            xxx::label("Content b", 0xC7F464_c);
            xxx::label("Content c0", 0x556270_c);
            xxx::label("Content c1", xxx::makeColor(85, 98, 112));
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
            xxx::spinner(spinnerStepStorage, "Loading", xxx::Align::Center);
          xxx::panelEnd();
      xxx::rowEnd();

      if (showTextEnter) {
        xxx::fixedPanelBegin({10, 5, 20, 20});
          xxx::label("Enter a message:");
          if (xxx::textInput(text)) {
            text.clear();
            showTextEnter = false;
          }
        xxx::fixedPanelEnd();
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
