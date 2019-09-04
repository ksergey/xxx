// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#ifndef KSERGEY_key_040919102840
#define KSERGEY_key_040919102840

#include <cstdint>

namespace xxx {

/// Keys.
enum class key : std::uint16_t {
  esc = 0x1B,
  F1 = (0xFFFF - 0),
  F2 = (0xFFFF - 1),
  F3 = (0xFFFF - 2),
  F4 = (0xFFFF - 3),
  F5 = (0xFFFF - 4),
  F6 = (0xFFFF - 5),
  F7 = (0xFFFF - 6),
  F8 = (0xFFFF - 7),
  F9 = (0xFFFF - 8),
  F10 = (0xFFFF - 9),
  F11 = (0xFFFF - 10),
  F12 = (0xFFFF - 11),
  insert = (0xFFFF - 12),
  delete_ = (0xFFFF - 13),
  home = (0xFFFF - 14),
  end = (0xFFFF - 15),
  pgup = (0xFFFF - 16),
  pgdn = (0xFFFF - 17),
  arrow_up = (0xFFFF - 18),
  arrow_down = (0xFFFF - 19),
  arrow_left = (0xFFFF - 20),
  arrow_right = (0xFFFF - 21)
};

}  // namespace xxx

#endif /* KSERGEY_key_040919102840 */
