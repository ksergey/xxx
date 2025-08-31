// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include "xxx2.h"

namespace xxx {

class im_io {
private:
public:
  im_io(im_io const&) = delete;
  im_io& operator=(im_io const&) = delete;
  im_io() = default;

  void add_key_event(im_key key);
  void add_input_character(std::uint32_t ch);
};

} // namespace xxx
