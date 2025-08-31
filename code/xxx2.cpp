// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "xxx2.h"

#include <stdexcept>

#include "xxx_internal.h"
#include "xxx_io.h"

namespace xxx {

void create_context() {
  // TODO: return std::expected?

  // init termbox2 library
  if (auto const rc = ::tb_init(); rc != TB_OK) {
    throw std::runtime_error(::tb_strerror(rc));
  }
  ::tb_set_output_mode(TB_OUTPUT_TRUECOLOR);

  ctx = new im_context;
}

void destroy_context() {
  delete ctx;

  ::tb_shutdown();
}

void poll_events(std::chrono::milliseconds timeout) {
  auto const start = clock::now();
  auto const expiration = start + timeout;

  ::tb_event event;
  while (true) {
    auto const rc = ::tb_peek_event(&event, static_cast<int>(timeout.count()));
    if (rc == TB_OK) {
      switch (event.type) {
      case TB_EVENT_KEY: {
        if (event.ch > 0) {
          ctx->io.add_input_character(event.ch);
        } else if (event.key > 0) {
          ctx->io.add_key_event(im_key(event.key));
        }
      } break;
      case TB_EVENT_MOUSE: {
      } break;
      case TB_EVENT_RESIZE: {
      } break;
      default:
        break;
      };
    } else if (rc == TB_ERR_NO_EVENT) {
      // nothing to do
    } else if (!(rc == TB_ERR_POLL && tb_last_errno() == EINTR)) {
      throw std::runtime_error(::tb_strerror(rc));
    }

    auto const now = clock::now();
    if (now >= expiration) {
      break;
    }

    timeout = std::chrono::duration_cast<std::chrono::milliseconds>(expiration - now);
  }

  // what next
}

void new_frame() {}

void render() {}

} // namespace xxx
