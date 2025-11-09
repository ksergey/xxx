// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "im_rect.h"

namespace xxx {

static_assert(im_rect().null());
static_assert(im_rect().empty());
static_assert(!im_rect().valid());

static_assert(im_rect(0, 0, 4, 2).width() == 5);
static_assert(im_rect(0, 0, 4, 2).height() == 3);
static_assert(im_rect(0, 0, 4, 2).size() == im_vec2(5, 3));

static_assert(im_rect(0, 0, 4, 2).contains(im_vec2(0, 0)));
static_assert(im_rect(0, 0, 4, 2).contains(im_vec2(1, 1)));
static_assert(im_rect(0, 0, 4, 2).contains(im_vec2(2, 2)));
static_assert(im_rect(0, 0, 4, 2).contains(im_vec2(3, 2)));
static_assert(im_rect(0, 0, 4, 2).contains(im_vec2(4, 2)));
static_assert(im_rect(0, 0, 4, 2).contains(im_vec2(3, 1)));

static_assert(!im_rect(0, 0, 4, 2).contains(im_vec2(-1, 0)));
static_assert(!im_rect(0, 0, 4, 2).contains(im_vec2(-1, -1)));
static_assert(!im_rect(0, 0, 4, 2).contains(im_vec2(4, 3)));
static_assert(!im_rect(0, 0, 4, 2).contains(im_vec2(5, 2)));

static_assert(im_rect(0, 0, 4, 2).contains(im_rect(0, 0, 4, 2)));
static_assert(im_rect(0, 0, 4, 2).contains(im_rect(1, 1, 3, 1)));
static_assert(!im_rect(0, 0, 4, 2).contains(im_rect(1, 1, 5, 3)));
static_assert(!im_rect(0, 0, 4, 2).contains(im_rect(-1, -1, 3, 1)));

static_assert(im_rect(0, 0, 4, 2).intersection(im_rect(0, 0, 4, 2)) == im_rect(0, 0, 4, 2));
static_assert(im_rect(0, 0, 4, 2).intersection(im_rect(1, 1, 4, 2)) == im_rect(1, 1, 4, 2));
static_assert(im_rect(0, 0, 4, 2).intersection(im_rect(-1, -1, 5, 3)) == im_rect(0, 0, 4, 2));
static_assert(im_rect(0, 0, 4, 2).intersection(im_rect(-1, -1, 3, 1)) == im_rect(0, 0, 3, 1));

static_assert(im_rect(0, 0, 4, 2).intersection(im_rect(-1, -1, 0, 0)) == im_rect(0, 0, 0, 0));
// static_assert(im_rect(0, 0, 4, 2).intersection(im_rect(-1, -1, 0, 0)).empty());

} // namespace xxx
