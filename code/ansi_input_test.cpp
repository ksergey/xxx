// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <string>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

#include "ansi_input.h"
#include "test_utils.h"

namespace xxx::testing {

using namespace std::string_view_literals;

namespace {

// readable event list: "k:enter", "c:x", ...
auto events(im_input const& in) -> std::vector<std::string> {
    auto result = std::vector<std::string>();
    for (auto const& e : in.get_input_events()) {
        result.push_back(
            e.ch ? "c:" + std::string(1, e.ch < 0x80 ? char(e.ch) : '?') : "k:" + std::to_string(int(e.key)));
    }
    return result;
}

auto key(im_key_id id) -> std::string {
    return "k:" + std::to_string(int(id));
}

// parse whole input at once and byte by byte: both must give the same events
auto parse(std::string_view bytes) -> std::vector<std::string> {
    ansi_input_parser whole, split;
    im_input a, b;
    whole.feed(bytes, a);
    whole.flush(a);
    for (auto const ch : bytes) {
        split.feed(std::string_view(&ch, 1), b);
    }
    split.flush(b);
    auto const ea = events(a);
    CHECK(ea == events(b));
    return ea;
}

} // namespace

TEST_SUITE("ansi input: keys") {

    TEST_CASE("text and space") {
        CHECK(parse("a b") == std::vector<std::string>{"c:a", "c: ", key(im_key_id::space), "c:b"});
    }

    TEST_CASE("utf8 text split anywhere") {
        ansi_input_parser p;
        im_input in;
        auto const text = "жü😀"sv;
        for (auto const ch : text) {
            p.feed(std::string_view(&ch, 1), in);
        }
        auto const e = in.get_input_events();
        REQUIRE(e.size() == 3);
        CHECK(e[0].ch == 0x436);
        CHECK(e[1].ch == 0xfc);
        CHECK(e[2].ch == 0x1f600);
    }

    TEST_CASE("control keys") {
        CHECK(parse("\r\t\x7f\x08"sv) == std::vector<std::string>{key(im_key_id::enter), key(im_key_id::tab),
                                             key(im_key_id::backspace2), key(im_key_id::backspace)});
        CHECK(parse("\x01\x0a\x0b\x0e\x1a"sv) == std::vector<std::string>{key(im_key_id::ctrl_a),
                                                     key(im_key_id::ctrl_j), key(im_key_id::ctrl_k),
                                                     key(im_key_id::ctrl_n), key(im_key_id::ctrl_z)});
    }

    TEST_CASE("unsupported control chars are ignored") {
        CHECK(parse("\x0c\x00\x1c"sv).empty()); // ctrl-l, NUL, ctrl-\ have no im_key_id
    }

    TEST_CASE("CSI keys, modifiers ignored") {
        CHECK(parse("\x1b[A\x1b[B\x1b[C\x1b[D") == std::vector<std::string>{key(im_key_id::arrow_up),
                                                       key(im_key_id::arrow_down), key(im_key_id::arrow_right),
                                                       key(im_key_id::arrow_left)});
        CHECK(parse("\x1b[1;5C") == std::vector<std::string>{key(im_key_id::arrow_right)}); // ctrl+right
        CHECK(parse("\x1b[H\x1b[F\x1b[Z") ==
              std::vector<std::string>{key(im_key_id::home), key(im_key_id::end), key(im_key_id::back_tab)});
    }

    TEST_CASE("CSI ~ keys") {
        CHECK(
            parse("\x1b[1~\x1b[4~\x1b[7~\x1b[8~") == std::vector<std::string>{key(im_key_id::home), key(im_key_id::end),
                                                         key(im_key_id::home), key(im_key_id::end)});
        CHECK(parse("\x1b[3~\x1b[5~\x1b[6~") ==
              std::vector<std::string>{key(im_key_id::del), key(im_key_id::page_up), key(im_key_id::page_down)});
    }

    TEST_CASE("SS3 keys (application mode)") {
        CHECK(parse("\x1bOA\x1bOH\x1bOF") ==
              std::vector<std::string>{key(im_key_id::arrow_up), key(im_key_id::home), key(im_key_id::end)});
    }

    TEST_CASE("unknown sequences are swallowed") {
        // insert, device attributes reply, keypad enter in application mode
        CHECK(parse("\x1b[2~x\x1b[?1;2cy\x1bOMz") == std::vector<std::string>{"c:x", "c:y", "c:z"});
    }

    TEST_CASE("function keys") {
        CHECK(parse("\x1bOP\x1bOQ\x1bOR\x1bOS") ==
              std::vector<std::string>{key(im_key_id::f1), key(im_key_id::f2), key(im_key_id::f3), key(im_key_id::f4)});
        CHECK(parse("\x1b[1;2P\x1b[1;5S") ==
              std::vector<std::string>{key(im_key_id::f1), key(im_key_id::f4)}); // modifiers
        CHECK(parse("\x1b[11~\x1b[15~\x1b[17~\x1b[21~\x1b[23~\x1b[24~") ==
              std::vector<std::string>{key(im_key_id::f1), key(im_key_id::f5), key(im_key_id::f6), key(im_key_id::f10),
                  key(im_key_id::f11), key(im_key_id::f12)});
        CHECK(parse("\x1b[15;2~") == std::vector<std::string>{key(im_key_id::f5)});
    }
}

TEST_SUITE("ansi input: esc") {

    TEST_CASE("lone esc waits, flush makes it the Esc key") {
        ansi_input_parser p;
        im_input in;
        CHECK_FALSE(p.feed("\x1b", in));
        CHECK(p.pending());
        CHECK(in.get_input_events().empty());
        CHECK(p.flush(in));
        CHECK(in.is_key_pressed(im_key_id::esc));
        CHECK_FALSE(p.pending());
    }

    TEST_CASE("esc followed by sequence in the next read is not Esc") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b", in);
        p.feed("[A", in);
        CHECK(in.is_key_pressed(im_key_id::arrow_up));
        CHECK_FALSE(in.is_key_pressed(im_key_id::esc));
    }

    TEST_CASE("double esc: first is a key") {
        CHECK(parse("\x1b\x1b") == std::vector<std::string>{key(im_key_id::esc), key(im_key_id::esc)});
    }

    TEST_CASE("alt+char keeps the char") {
        CHECK(parse("\x1bx") == std::vector<std::string>{"c:x"});
    }

    TEST_CASE("unfinished sequence is dropped on flush") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[1;5", in);
        CHECK(p.pending());
        CHECK_FALSE(p.flush(in));
        CHECK(in.get_input_events().empty());
        p.feed("a", in); // parser is clean again
        CHECK(in.get_input_events().size() == 1);
    }

    TEST_CASE("over-long sequence is garbage however it is split") {
        // parse() checks that one chunk and byte by byte give the same result
        auto const e = parse("\x1b[" + std::string(100, '1') + "x");
        REQUIRE(e.size() == 102);
        CHECK(e.front() == "c:[");
        CHECK(e.back() == "c:x");
    }

    TEST_CASE("long but sane sequence is still a sequence") {
        CHECK(parse("\x1b[" + std::string(60, '1') + "x").empty());
    }

    TEST_CASE("growing sequence without a final byte does not block input forever") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[" + std::string(100, '1'), in); // no final byte: dropped as garbage after 64 bytes
        // the ESC was dropped, the rest was read as text: input is not stuck
        CHECK_FALSE(in.get_input_events().empty());
        in.reset();
        p.feed("\r", in);
        CHECK(in.is_key_pressed(im_key_id::enter));
    }
}

TEST_SUITE("ansi input: mouse") {

    TEST_CASE("press, coordinates are 0-based") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[<0;10;5M", in);
        CHECK(in.is_mouse_clicked(im_mouse_button_id::left));
        CHECK(in.mouse_clicked_pos(im_mouse_button_id::left) == im_vec2(9, 4));
    }

    TEST_CASE("middle and right buttons") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[<1;1;1M\x1b[<2;2;2M", in);
        CHECK(in.is_mouse_clicked(im_mouse_button_id::middle));
        CHECK(in.is_mouse_clicked(im_mouse_button_id::right));
        CHECK_FALSE(in.is_mouse_clicked(im_mouse_button_id::left));
    }

    TEST_CASE("release and drag are not clicks") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[<0;3;3m\x1b[<32;4;3M\x1b[<35;7;8M", in); // release, drag with left, motion without button
        CHECK_FALSE(in.is_mouse_clicked(im_mouse_button_id::left));
        CHECK(in.mouse_pos() == im_vec2(6, 7));
    }

    TEST_CASE("wheel") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[<65;5;5M\x1b[<65;5;5M\x1b[<64;5;5M", in);
        CHECK(in.mouse_wheel() == 1);
    }

    TEST_CASE("split mouse sequence") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[<0;1", in);
        CHECK(p.pending());
        p.feed("2;3M", in);
        CHECK(in.mouse_clicked_pos(im_mouse_button_id::left) == im_vec2(11, 2));
    }
}

TEST_SUITE("ansi input: bracketed paste") {

    TEST_CASE("pasted text is characters, newline does not press enter") {
        auto const e = parse("\x1b[200~ab\ncd\r\tx\x1b[201~");
        CHECK(e == std::vector<std::string>{"c:a", "c:b", "c: ", "c:c", "c:d", "c: ", "c: ", "c:x"});
    }

    TEST_CASE("escape sequences inside paste are text") {
        CHECK(parse("\x1b[200~\x1b[Aq\x1b[201~") == std::vector<std::string>{"c:[", "c:A", "c:q"});
    }

    TEST_CASE("keys after paste work again") {
        CHECK(parse("\x1b[200~p\x1b[201~\r") == std::vector<std::string>{"c:p", key(im_key_id::enter)});
    }

    TEST_CASE("terminator split between reads") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[200~hello\x1b[20", in);
        CHECK(in.get_input_events().size() == 5);
        p.feed("1~\r", in);
        CHECK(in.is_key_pressed(im_key_id::enter));
        CHECK(in.get_input_events().size() == 6);
    }

    TEST_CASE("flush does not end paste") {
        ansi_input_parser p;
        im_input in;
        p.feed("\x1b[200~ab", in);
        p.flush(in);
        p.feed("\rc\x1b[201~", in);
        CHECK_FALSE(in.is_key_pressed(im_key_id::enter));
        CHECK(in.get_input_events().size() == 4);
    }
}

} // namespace xxx::testing
