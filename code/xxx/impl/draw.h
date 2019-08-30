#ifndef KSERGEY_draw_300819110406
#define KSERGEY_draw_300819110406

#include <memory>

#include <xxx/ui.h>

#include <termbox.h>

#include "compiler.h"

namespace xxx::impl {

using cell = ::tb_cell;

XXX_ALWAYS_INLINE cell make_cell(std::uint32_t ch, color fg = color::default_, color bg = color::default_) noexcept {
  return {ch, static_cast<std::uint16_t>(fg), static_cast<std::uint16_t>(bg)};
}

XXX_ALWAYS_INLINE void draw_cell(int x, int y, cell const& c) noexcept { ::tb_put_cell(x, y, std::addressof(c)); }

XXX_ALWAYS_INLINE void draw_horizontal_line(int x, int y, int length, cell const& c) noexcept {
  while (length-- > 0) {
    draw_cell(x, y, c);
    x += 1;
  }
}

XXX_ALWAYS_INLINE void draw_vertical_line(int x, int y, int length, cell const& c) noexcept {
  while (length-- > 0) {
    draw_cell(x, y, c);
    y += 1;
  }
}

XXX_ALWAYS_INLINE void draw_text(int x, int y, char const* str, int length, color fg = color::default_,
                                 color bg = color::default_) {
  auto cell = make_cell(' ', fg, bg);
  while (length > 0) {
    auto rc = ::tb_utf8_char_to_unicode(&cell.ch, str);
    if (rc == TB_EOF) {
      break;
    }
    draw_cell(x++, y, cell);
    length -= 1;
    str += rc;
  }
}

}  // namespace xxx::impl

#endif /* KSERGEY_draw_300819110406 */
