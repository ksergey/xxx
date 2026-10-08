// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "im_renderer.h"
#include "test_utils.h"

namespace xxx::testing {

TEST_SUITE("im_renderer") {

    TEST_CASE("start_new_frame resets clip and viewport") {
        im_renderer r;
        r.set_viewport_offset(im_vec2(3, 3));
        r.start_new_frame(im_rect(0, 0, 79, 23));
        CHECK(r.clip_rect() == im_rect(0, 0, 79, 23));
        CHECK(r.viewport_offset() == im_vec2(0, 0));
    }

    TEST_CASE("clip rect stack") {
        im_renderer r;
        r.start_new_frame(im_rect(0, 0, 79, 23));

        SUBCASE("push intersects with parent") {
            r.push_clip_rect(im_rect(70, 20, 100, 100));
            CHECK(r.clip_rect() == im_rect(70, 20, 79, 23));
            r.pop_clip_rect();
            CHECK(r.clip_rect() == im_rect(0, 0, 79, 23));
        }
        SUBCASE("push without clip_to_parent") {
            r.push_clip_rect(im_rect(70, 20, 100, 100), false);
            CHECK(r.clip_rect() == im_rect(70, 20, 100, 100));
        }
        SUBCASE("nested") {
            r.push_clip_rect(im_rect(10, 10, 50, 20));
            r.push_clip_rect(im_rect(0, 0, 20, 15));
            CHECK(r.clip_rect() == im_rect(10, 10, 20, 15));
            r.pop_clip_rect();
            CHECK(r.clip_rect() == im_rect(10, 10, 50, 20));
            r.pop_clip_rect();
            CHECK(r.clip_rect() == im_rect(0, 0, 79, 23));
        }
        SUBCASE("pop on empty stack is no-op") {
            r.pop_clip_rect();
            CHECK(r.clip_rect() == im_rect(0, 0, 79, 23));
        }
        SUBCASE("start_new_frame clears stack") {
            r.push_clip_rect(im_rect(1, 1, 2, 2));
            r.start_new_frame(im_rect(0, 0, 9, 9));
            r.pop_clip_rect();
            CHECK(r.clip_rect() == im_rect(0, 0, 9, 9));
        }
    }

    TEST_CASE("visibility") {
        im_renderer r;
        r.start_new_frame(im_rect(0, 0, 9, 9));

        CHECK(r.is_visible(im_vec2(0, 0)));
        CHECK(r.is_visible(im_vec2(9, 9)));
        CHECK_FALSE(r.is_visible(im_vec2(10, 0)));
        CHECK(r.is_visible(im_rect(8, 8, 20, 20)));
        CHECK_FALSE(r.is_visible(im_rect(10, 10, 20, 20)));

        SUBCASE("viewport offset shifts content") {
            r.set_viewport_offset(im_vec2(5, 0));
            CHECK(r.is_visible(im_vec2(14, 0)));
            CHECK_FALSE(r.is_visible(im_vec2(4, 0)));
            CHECK(r.is_visible(im_rect(10, 0, 12, 0)));
        }
    }
}

} // namespace xxx::testing
