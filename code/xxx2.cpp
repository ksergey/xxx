// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx2.h"

#include "xxx_internal.h"

namespace xxx {

auto create_context() -> std::expected<void, std::string_view> {
  // init termbox2 library
  if (auto const rc = ::tb_init(); rc != TB_OK) {
    return std::unexpected(::tb_strerror(rc));
  }
  ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);

  return {};
}

void destroy_context() {
  ::tb_shutdown();
}

void poll_events() {
  // TODO
  ::tb_event event;
  // auto const rc = ::tb_peek_event(&event, static_cast<int>(timeoutMs));
  // if (rc != TB_OK && rc != TB_ERR_NO_EVENT) [[unlikely]] {
  //   if (!(rc == TB_ERR_POLL && tb_last_errno() == EINTR)) {
  //     throw std::runtime_error(::tb_strerror(rc));
  //   }
  // }
}

void new_frame() {}

void render() {}

} // namespace xxx
