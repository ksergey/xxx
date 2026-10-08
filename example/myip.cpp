// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// External IP address as seen by several services in different domain zones. All of them are asked at
// once (curl multi in a background thread); each answer shows up as soon as it arrives. Errors are red,
// addresses that differ from what most services say are yellow.
//
//   myip [-4 | -6] [-t SECONDS] [-u URL]...

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <format>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <curl/curl.h>
#include <cxxopts.hpp>

#include <xxx.h>

namespace {

using namespace std::chrono_literals;
using namespace std::string_view_literals;

// plain text answers, one domain zone each
constexpr auto default_services = std::to_array<std::string_view>({
    "https://api.ipify.org",
    "https://ifconfig.me/ip",
    "https://icanhazip.com",
    "https://ipinfo.io/ip",
    "https://ipecho.net/plain",
    "https://ifconfig.co/ip",
    "https://ip.sb",
    "https://ip.tyk.nu",
    "http://2ip.ru", // plain text for curl's User-Agent; documented as plain http
});

struct options {
    std::vector<std::string> urls;
    long ip_resolve = CURL_IPRESOLVE_WHATEVER;
    long timeout_s = 10;
};

[[nodiscard]] auto parse_options(int argc, char** argv) -> std::optional<options> {
    auto cli = cxxopts::Options("myip", "External IP address from several services in different domain zones");
    cli.add_options()("4,ipv4", "IPv4 only")("6,ipv6", "IPv6 only")(
        "t,timeout", "seconds per service", cxxopts::value<long>()->default_value("10"), "SECONDS")("u,url",
        "ask this service instead of the built-in ones (repeatable)", cxxopts::value<std::vector<std::string>>(),
        "URL")("h,help", "print this help");
    try {
        auto const args = cli.parse(argc, argv);
        if (args.count("help")) {
            std::printf("%s\n", cli.help().c_str());
            return std::nullopt;
        }
        auto opts = options();
        opts.timeout_s = std::max(1L, args["timeout"].as<long>());
        if (args.count("ipv4")) {
            opts.ip_resolve = CURL_IPRESOLVE_V4;
        } else if (args.count("ipv6")) {
            opts.ip_resolve = CURL_IPRESOLVE_V6;
        }
        if (args.count("url")) {
            opts.urls = args["url"].as<std::vector<std::string>>();
        } else {
            opts.urls.assign(default_services.begin(), default_services.end());
        }
        return opts;
    } catch (cxxopts::exceptions::exception const& e) {
        std::fprintf(stderr, "myip: %s\n", e.what());
        return std::nullopt;
    }
}

// "https://api.ipify.org/x" -> host "api.ipify.org", zone ".org"
[[nodiscard]] auto host_of(std::string_view url) -> std::string_view {
    if (auto const scheme = url.find("://"); scheme != std::string_view::npos) {
        url.remove_prefix(scheme + 3);
    }
    return url.substr(0, url.find_first_of("/:?"));
}

// empty for an IP address or a host without dots (localhost)
[[nodiscard]] auto zone_of(std::string_view host) -> std::string_view {
    auto const text = std::string(host.starts_with('[') ? host.substr(1, host.size() - 2) : host);
    unsigned char buffer[16];
    if (::inet_pton(AF_INET, text.c_str(), buffer) == 1 || ::inet_pton(AF_INET6, text.c_str(), buffer) == 1) {
        return {};
    }
    auto const dot = host.rfind('.');
    return dot == std::string_view::npos ? std::string_view() : host.substr(dot);
}

// an answer is an address, not an HTML page or an error text
[[nodiscard]] auto parse_address(std::string_view body) -> std::optional<std::string> {
    auto const begin = body.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos) {
        return std::nullopt;
    }
    body.remove_prefix(begin);
    body = body.substr(0, body.find_first_of(" \t\r\n"));
    auto const text = std::string(body);
    unsigned char buffer[16];
    if (::inet_pton(AF_INET, text.c_str(), buffer) == 1 || ::inet_pton(AF_INET6, text.c_str(), buffer) == 1) {
        return text;
    }
    return std::nullopt;
}

struct result {
    enum class state { pending, done, failed } status = state::pending;
    std::string address;
    std::string error;
    std::chrono::milliseconds elapsed{};
};

// asks all services at once; each finished request updates its row and wakes the ui up
class checker {
public:
    explicit checker(options opts) : opts_(std::move(opts)), results_(opts_.urls.size()) {}

    ~checker() {
        stop_ = true;
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    [[nodiscard]] auto urls() const -> std::vector<std::string> const& {
        return opts_.urls;
    }

    [[nodiscard]] auto running() const -> bool {
        return running_;
    }

    [[nodiscard]] auto copy() const -> std::vector<result> {
        auto lock = std::lock_guard(mutex_);
        return results_;
    }

    void start() {
        if (running_) {
            return;
        }
        if (worker_.joinable()) {
            worker_.join();
        }
        {
            auto lock = std::lock_guard(mutex_);
            std::fill(results_.begin(), results_.end(), result());
        }
        running_ = true;
        worker_ = std::thread([this] {
            run();
        });
    }

private:
    struct transfer {
        std::size_t index;
        std::string body;
        char error[CURL_ERROR_SIZE] = {};
    };

    void run() {
        auto* const multi = ::curl_multi_init();
        auto transfers = std::vector<transfer>(opts_.urls.size());
        auto handles = std::vector<CURL*>();
        auto const user_agent = std::format("curl/{}", ::curl_version_info(CURLVERSION_NOW)->version);

        for (std::size_t i = 0; i < opts_.urls.size(); ++i) {
            auto& t = transfers[i];
            t.index = i;
            auto* const h = ::curl_easy_init();
            ::curl_easy_setopt(h, CURLOPT_URL, opts_.urls[i].c_str());
            // some services answer in plain text only to curl
            ::curl_easy_setopt(h, CURLOPT_USERAGENT, user_agent.c_str());
            ::curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
            ::curl_easy_setopt(h, CURLOPT_IPRESOLVE, opts_.ip_resolve);
            ::curl_easy_setopt(h, CURLOPT_TIMEOUT, opts_.timeout_s);
            ::curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, std::min(opts_.timeout_s, 5L));
            ::curl_easy_setopt(h, CURLOPT_NOSIGNAL, 1L);
            ::curl_easy_setopt(h, CURLOPT_ERRORBUFFER, t.error);
            ::curl_easy_setopt(h, CURLOPT_WRITEDATA, &t.body);
            ::curl_easy_setopt(
                h, CURLOPT_WRITEFUNCTION, +[](char* data, std::size_t, std::size_t n, void* user) {
                    auto& body = *static_cast<std::string*>(user);
                    body.append(data, std::min<std::size_t>(
                                          n, 4096 - std::min<std::size_t>(body.size(), 4096))); // enough for an address
                    return n;
                });
            ::curl_easy_setopt(h, CURLOPT_PRIVATE, &t);
            ::curl_multi_add_handle(multi, h);
            handles.push_back(h);
        }

        auto active = 1;
        while (active > 0 && !stop_) {
            ::curl_multi_perform(multi, &active);
            auto left = 0;
            while (auto const* const message = ::curl_multi_info_read(multi, &left)) {
                if (message->msg != CURLMSG_DONE) {
                    continue;
                }
                finish(message->easy_handle, message->data.result);
            }
            if (active > 0) {
                ::curl_multi_poll(multi, nullptr, 0, 200, nullptr); // wakes up on network activity, checks stop_
            }
        }

        for (auto* const h : handles) {
            ::curl_multi_remove_handle(multi, h);
            ::curl_easy_cleanup(h);
        }
        ::curl_multi_cleanup(multi);
        running_ = false;
        if (!stop_) {
            xxx::wake_up();
        }
    }

    void finish(CURL* handle, CURLcode code) {
        transfer* t = nullptr;
        ::curl_easy_getinfo(handle, CURLINFO_PRIVATE, &t);
        auto status = 0L;
        ::curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);
        auto seconds = 0.0;
        ::curl_easy_getinfo(handle, CURLINFO_TOTAL_TIME, &seconds);

        auto r = result();
        r.elapsed = std::chrono::milliseconds(long(seconds * 1000));
        if (code != CURLE_OK) {
            r.status = result::state::failed;
            r.error = t->error[0] ? t->error : ::curl_easy_strerror(code);
        } else if (status != 200) {
            r.status = result::state::failed;
            r.error = std::format("HTTP {}", status);
        } else if (auto address = parse_address(t->body)) {
            r.status = result::state::done;
            r.address = std::move(*address);
        } else {
            r.status = result::state::failed;
            auto snippet = t->body.substr(0, 40);
            std::replace_if(
                snippet.begin(), snippet.end(),
                [](char c) {
                    return c == '\n' || c == '\r' || c == '\t';
                },
                ' ');
            r.error = std::format("not an IP address: \"{}\"", snippet);
        }
        {
            auto lock = std::lock_guard(mutex_);
            results_[t->index] = std::move(r);
        }
        xxx::wake_up(); // this row shows up now, not when every service has answered
    }

    options opts_;
    mutable std::mutex mutex_;
    std::vector<result> results_;
    std::atomic<bool> running_ = false;
    std::atomic<bool> stop_ = false;
    std::thread worker_;
};

// the address most services agree on and how many of them; a tie is no answer (empty address)
struct verdict {
    std::string address;
    int agreed = 0;
    int answers = 0; // services that returned an address
    bool tie = false;
};

[[nodiscard]] auto consensus(std::vector<result> const& results) -> verdict {
    auto votes = std::map<std::string, int>();
    auto v = verdict();
    for (auto const& r : results) {
        if (r.status == result::state::done) {
            ++votes[r.address];
            ++v.answers;
        }
    }
    for (auto const& [address, count] : votes) {
        if (count > v.agreed) {
            v = {.address = address, .agreed = count, .answers = v.answers, .tie = false};
        } else if (count == v.agreed) {
            v.tie = true;
        }
    }
    if (v.tie) {
        v.address.clear();
    }
    return v;
}

} // namespace

int main(int argc, char** argv) {
    auto const opts = parse_options(argc, argv);
    if (!opts) {
        return argc > 1 && std::string_view(argv[1]) != "-h" && std::string_view(argv[1]) != "--help" ? 2 : 0;
    }
    ::curl_global_init(CURL_GLOBAL_DEFAULT);
    xxx::init();
    {
        auto checker = ::checker(*opts);
        checker.start();
        auto spinner = 0.0f;
        auto zones = std::map<std::string_view, int>();
        for (auto const& url : checker.urls()) {
            if (auto const zone = zone_of(host_of(url)); !zone.empty()) {
                ++zones[zone];
            }
        }

        while (true) {
            // spinners need ticks while requests run; otherwise sleep until a key or a result
            xxx::process_input_events(checker.running() ? 100ms : xxx::wait_forever);
            if (xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
                break;
            }
            auto const results = checker.copy();
            auto const verdict = consensus(results);
            auto const& address = verdict.address;
            auto const finished = std::count_if(results.begin(), results.end(), [](auto const& r) {
                return r.status != result::state::pending;
            });

            // plain letters are safe shortcuts when no text input has focus (there is none here anyway)
            auto ask_again = !xxx::wants_text_input() && xxx::is_char_pressed('r');
            if (!xxx::wants_text_input() && xxx::is_char_pressed('c') && !address.empty()) {
                xxx::set_clipboard(address);
            }

            xxx::new_frame();
            xxx::view_begin("external ip", xxx::im_view_flags_default, xxx::im_key_id(), xxx::fill(1));
            xxx::label(std::format("{} services in {} domain zones · {} of {} answered", results.size(), zones.size(),
                finished, results.size()));
            if (verdict.tie) {
                // no majority: don't pick one by chance
                xxx::label(std::format("services disagree: {} addresses, {} answer{} each",
                               verdict.answers / verdict.agreed, verdict.agreed, verdict.agreed == 1 ? "" : "s"),
                    xxx::im_role::warning);
            } else if (verdict.agreed == 0) {
                xxx::label(finished == std::ssize(results) ? "no service could tell the address" : "asking...",
                    finished == std::ssize(results) ? xxx::im_role::error : xxx::im_role::muted);
            } else {
                xxx::label(std::format("your ip: {}  (agreed by {} of {})", address, verdict.agreed, verdict.answers),
                    xxx::im_role::success);
            }
            xxx::label(" ");

            for (std::size_t i = 0; i < results.size(); ++i) {
                auto const& r = results[i];
                auto const host = host_of(checker.urls()[i]);
                xxx::push_id(int(i));
                xxx::layout_row_begin(4);
                xxx::layout_row_push(xxx::cells(20));
                xxx::label(host);
                xxx::layout_row_push(xxx::cells(5));
                auto const zone = zone_of(host);
                xxx::label(zone.empty() ? "-"sv : zone, xxx::im_role::muted);
                xxx::layout_row_push(xxx::cells(8));
                xxx::label(r.status == result::state::pending ? "" : std::format("{} ms", r.elapsed.count()),
                    xxx::im_role::muted);
                xxx::layout_row_push(xxx::fill());
                switch (r.status) {
                case result::state::pending: xxx::spinner("", spinner); break;
                case result::state::done:
                    // an address that differs from the majority is suspicious
                    xxx::label(r.address, r.address == address ? xxx::im_role::text : xxx::im_role::warning);
                    break;
                case result::state::failed: xxx::label(r.error, xxx::im_role::error); break;
                }
                xxx::layout_row_end();
                xxx::pop_id();
            }
            xxx::label(" ");
            ask_again = xxx::button(checker.running() ? "asking##refresh" : "ask again##refresh") || ask_again;
            if (ask_again) {
                checker.start(); // does nothing while a round is running
            }
            xxx::view_end();

            xxx::key_hint("r", "ask all services again");
            xxx::key_hint("c", "copy the address");
            xxx::key_hint("c-q", "quit");
            xxx::key_hints("r again · c copy · c-q quit");
            xxx::render();
        }
    }
    xxx::shutdown();
    ::curl_global_cleanup();
    return 0;
}
