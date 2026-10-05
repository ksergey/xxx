// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

// Terminal dashboard for mihomo (Clash.Meta) through its RESTful API (external-controller):
// traffic, proxy groups (select a proxy, test delays) and active connections (close them).
//
//   mihomo [-u URL] [-s SECRET] [-k] [--cacert FILE] [--delay-url URL]    (see --help)
//
// How it is built: the UI thread never touches the network. Three background threads talk to
// mihomo (libcurl handles are per thread) and put results into a shared snapshot:
//   poller   - GET /version, /proxies, /connections every second
//   traffic  - GET /traffic, a never ending stream with one JSON line per second
//   commands - select proxy, test delay, close connection (requests from the UI)
// After each update a thread calls xxx::wake_up(), so the UI sleeps in
// process_input_events(wait_forever) and redraws only when there is something new.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <expected>
#include <format>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <curl/curl.h>
#include <cxxopts.hpp>
#include <nlohmann/json.hpp>

#include <xxx.h>

namespace {

using namespace std::chrono_literals;
using namespace std::string_view_literals;
using namespace xxx::literals;
using json = nlohmann::json;

// ---------------------------------------------------------------------------------------------
// options

struct options {
  std::string url = "http://127.0.0.1:9090";
  std::string secret;
  std::string cacert;
  std::string delay_url = "https://www.gstatic.com/generate_204";
  bool insecure = false;
};

// command line first, then environment, then defaults.
// Environment is applied after parsing on purpose: as a cxxopts default value the secret would be
// printed by --help.
// @return options or the exit code (0 for --help)
[[nodiscard]] auto parse_options(int argc, char** argv) -> std::expected<options, int> {
  auto opts = options();
  auto cli = cxxopts::Options("mihomo", "Terminal dashboard for mihomo (Clash.Meta) RESTful API");
  // clang-format off
  cli.add_options()
      ("u,url", "controller address, http or https (env MIHOMO_URL)",
          cxxopts::value<std::string>()->default_value(opts.url))
      ("s,secret", "controller secret (env MIHOMO_SECRET)", cxxopts::value<std::string>())
      ("k,insecure", "don't verify TLS certificate (self-signed external-controller-tls)")
      ("cacert", "verify TLS certificate against this CA or self-signed certificate",
          cxxopts::value<std::string>(), "FILE")
      ("delay-url", "what mihomo fetches through a proxy to measure delay",
          cxxopts::value<std::string>()->default_value(opts.delay_url))
      ("h,help", "print this help");
  // clang-format on

  try {
    auto const args = cli.parse(argc, argv);
    if (args.count("help")) {
      std::fputs(cli.help().c_str(), stdout);
      return std::unexpected(0);
    }
    if (!args.unmatched().empty()) {
      throw cxxopts::exceptions::exception("unexpected argument '" + args.unmatched().front() + "'");
    }
    auto const env = [](char const* name) { return std::getenv(name); };
    opts.url = args.count("url") || !env("MIHOMO_URL") ? args["url"].as<std::string>() : env("MIHOMO_URL");
    if (args.count("secret")) {
      opts.secret = args["secret"].as<std::string>();
    } else if (auto const* secret = env("MIHOMO_SECRET")) {
      opts.secret = secret;
    }
    opts.insecure = args.count("insecure") > 0;
    if (args.count("cacert")) {
      opts.cacert = args["cacert"].as<std::string>();
    }
    opts.delay_url = args["delay-url"].as<std::string>();
  } catch (cxxopts::exceptions::exception const& e) {
    std::fprintf(stderr, "mihomo: %s\n\n%s", e.what(), cli.help().c_str());
    return std::unexpected(2);
  }

  while (!opts.url.empty() && opts.url.back() == '/') {
    opts.url.pop_back();
  }
  return opts;
}

// ---------------------------------------------------------------------------------------------
// HTTP client: thin libcurl wrapper, one per thread

class http_client {
public:
  struct response {
    long status = 0;   // HTTP status, 0 when the request failed before getting one
    std::string body;
    std::string error; // transport error (connection refused, TLS, timeout)
  };

  explicit http_client(options const& opts) : opts_(opts), curl_(::curl_easy_init()) {
    if (!opts.secret.empty()) {
      headers_ = ::curl_slist_append(headers_, ("Authorization: Bearer " + opts.secret).c_str());
    }
    headers_ = ::curl_slist_append(headers_, "Content-Type: application/json");
  }

  ~http_client() {
    ::curl_slist_free_all(headers_);
    ::curl_easy_cleanup(curl_);
  }

  http_client(http_client const&) = delete;
  http_client& operator=(http_client const&) = delete;

  /// Percent-encode one path segment (proxy names may contain spaces, CJK, emoji)
  [[nodiscard]] auto escape(std::string_view segment) -> std::string {
    auto* const p = ::curl_easy_escape(curl_, segment.data(), int(segment.size()));
    auto result = std::string(p);
    ::curl_free(p);
    return result;
  }

  auto request(char const* method, std::string const& path, std::string const& body = {},
      std::chrono::milliseconds timeout = 5s) -> response {
    auto result = response();
    setup(path, timeout);
    ::curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, method);
    if (!body.empty()) {
      ::curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
      ::curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, long(body.size()));
    }
    ::curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &append_body);
    ::curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &result.body);
    finish(result);
    return result;
  }

  /// Streaming GET: on_line() gets every complete line until \c stop is set or the stream breaks
  auto stream(std::string const& path, std::function<void(std::string_view)> on_line, std::atomic<bool> const& stop)
      -> response {
    struct context {
      std::string pending;
      std::function<void(std::string_view)>* on_line;
      std::atomic<bool> const* stop;
    } ctx{.pending = {}, .on_line = &on_line, .stop = &stop};

    auto result = response();
    setup(path, 0ms); // no total timeout: the stream never ends by itself
    ::curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "GET");
    ::curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &ctx);
    ::curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, +[](char* data, std::size_t, std::size_t n, void* user) {
      auto& c = *static_cast<context*>(user);
      c.pending.append(data, n);
      for (auto eol = c.pending.find('\n'); eol != std::string::npos; eol = c.pending.find('\n')) {
        (*c.on_line)(std::string_view(c.pending).substr(0, eol));
        c.pending.erase(0, eol + 1);
      }
      return n;
    });
    // progress callback is called regularly even when no data comes: lets us abort on shutdown
    ::curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 0L);
    ::curl_easy_setopt(curl_, CURLOPT_XFERINFODATA, &stop);
    ::curl_easy_setopt(curl_, CURLOPT_XFERINFOFUNCTION,
        +[](void* user, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
          return static_cast<std::atomic<bool> const*>(user)->load() ? 1 : 0;
        });
    finish(result);
    ::curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 1L);
    return result;
  }

private:
  static auto append_body(char* data, std::size_t, std::size_t n, void* user) -> std::size_t {
    static_cast<std::string*>(user)->append(data, n);
    return n;
  }

  void setup(std::string const& path, std::chrono::milliseconds timeout) {
    ::curl_easy_reset(curl_);
    auto const url = opts_.url + path;
    ::curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    ::curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    ::curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, long(timeout.count()));
    ::curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, 3000L);
    ::curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L); // we are multi-threaded
    if (opts_.insecure) {
      ::curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
      ::curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    } else if (!opts_.cacert.empty()) {
      ::curl_easy_setopt(curl_, CURLOPT_CAINFO, opts_.cacert.c_str());
    }
  }

  void finish(response& result) {
    char error[CURL_ERROR_SIZE] = {};
    ::curl_easy_setopt(curl_, CURLOPT_ERRORBUFFER, error);
    if (auto const rc = ::curl_easy_perform(curl_); rc != CURLE_OK) {
      result.error = error[0] ? error : ::curl_easy_strerror(rc);
    }
    ::curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &result.status);
  }

  options const& opts_;
  CURL* curl_;
  curl_slist* headers_ = nullptr;
};

// ---------------------------------------------------------------------------------------------
// shared state: written by background threads, read by the UI under the mutex

struct proxy_info {
  std::string type;
  std::string now;              // groups: selected member
  std::vector<std::string> all; // groups: members
  int last_delay = 0;           // from history, 0 - not tested / failed
};

struct connection_info {
  std::string id;
  std::string host;
  std::string network;
  std::string chain;
  std::string rule;
  std::int64_t upload = 0;
  std::int64_t download = 0;
};

struct traffic_sample {
  std::int64_t up = 0;
  std::int64_t down = 0;
};

struct snapshot {
  bool connected = false;
  std::string error;
  std::string version;
  std::map<std::string, proxy_info> proxies;
  std::vector<std::string> groups; // display order
  std::vector<connection_info> connections;
  std::int64_t upload_total = 0;
  std::int64_t download_total = 0;
  std::deque<traffic_sample> traffic;       // newest at the back
  std::map<std::string, std::string> delay; // proxy -> "12 ms" / "testing" / error
};

constexpr auto traffic_history = std::size_t(240);

struct command {
  enum class kind { select, test_delay, close_connection } type;
  std::string first;  // group / proxy / connection id
  std::string second; // proxy to select
};

class mihomo_api {
public:
  explicit mihomo_api(options opts) : opts_(std::move(opts)) {
    poller_ = std::thread([this] { poll_loop(); });
    traffic_ = std::thread([this] { traffic_loop(); });
    commands_ = std::thread([this] { command_loop(); });
  }

  ~mihomo_api() {
    stop_ = true;
    queue_cv_.notify_all();
    poller_.join();
    traffic_.join();
    commands_.join();
  }

  [[nodiscard]] auto copy() const -> snapshot {
    auto lock = std::lock_guard(mutex_);
    return state_;
  }

  void post(command cmd) {
    {
      auto lock = std::lock_guard(queue_mutex_);
      queue_.push_back(std::move(cmd));
    }
    queue_cv_.notify_one();
  }

  [[nodiscard]] auto url() const -> std::string const& {
    return opts_.url;
  }

private:
  template <typename F>
  void update(F&& change) {
    {
      auto lock = std::lock_guard(mutex_);
      change(state_);
    }
    xxx::wake_up(); // UI thread redraws with the new data
  }

  // sleep up to \c duration, wake up early on shutdown
  void pause(std::chrono::milliseconds duration) {
    auto lock = std::unique_lock(queue_mutex_);
    queue_cv_.wait_for(lock, duration, [this] { return stop_.load(); });
  }

  [[nodiscard]] static auto describe(http_client::response const& r) -> std::string {
    if (!r.error.empty()) {
      return r.error;
    }
    if (r.status == 401) {
      return "401 unauthorized: check the secret (-s)";
    }
    auto message = std::string();
    if (auto const body = json::parse(r.body, nullptr, false); body.is_object() && body.contains("message")) {
      message = body["message"].get<std::string>();
    }
    return std::format("HTTP {} {}", r.status, message);
  }

  void poll_loop() {
    auto http = http_client(opts_);
    while (!stop_) {
      auto version = http.request("GET", "/version");
      if (version.status != 200) {
        update([&](snapshot& s) {
          s.connected = false;
          s.error = describe(version);
        });
        pause(2s);
        continue;
      }
      auto const proxies = http.request("GET", "/proxies");
      auto const connections = http.request("GET", "/connections");
      update([&](snapshot& s) {
        s.connected = true;
        s.error.clear();
        s.version = json::parse(version.body, nullptr, false).value("version", "?");
        if (proxies.status == 200) {
          read_proxies(json::parse(proxies.body, nullptr, false), s);
        }
        if (connections.status == 200) {
          read_connections(json::parse(connections.body, nullptr, false), s);
        }
      });
      pause(1s);
    }
  }

  static void read_proxies(json const& body, snapshot& s) {
    if (!body.is_object() || !body.contains("proxies")) {
      return;
    }
    s.proxies.clear();
    for (auto const& [name, p] : body["proxies"].items()) {
      auto& info = s.proxies[name];
      info.type = p.value("type", "");
      if (p.contains("now") && p["now"].is_string()) {
        info.now = p["now"].get<std::string>();
      }
      if (p.contains("all") && p["all"].is_array()) {
        info.all = p["all"].get<std::vector<std::string>>();
      }
      if (p.contains("history") && p["history"].is_array() && !p["history"].empty()) {
        info.last_delay = p["history"].back().value("delay", 0);
      }
    }
    // groups in the order mihomo shows them (members of GLOBAL), GLOBAL itself last
    s.groups.clear();
    if (auto const global = s.proxies.find("GLOBAL"); global != s.proxies.end()) {
      for (auto const& name : global->second.all) {
        if (auto const it = s.proxies.find(name); it != s.proxies.end() && !it->second.all.empty()) {
          s.groups.push_back(name);
        }
      }
      s.groups.push_back("GLOBAL");
    }
  }

  static void read_connections(json const& body, snapshot& s) {
    if (!body.is_object()) {
      return;
    }
    s.upload_total = body.value("uploadTotal", std::int64_t(0));
    s.download_total = body.value("downloadTotal", std::int64_t(0));
    s.connections.clear();
    if (!body.contains("connections") || !body["connections"].is_array()) {
      return; // null when there are none
    }
    for (auto const& c : body["connections"]) {
      auto info = connection_info();
      info.id = c.value("id", "");
      info.upload = c.value("upload", std::int64_t(0));
      info.download = c.value("download", std::int64_t(0));
      info.rule = c.value("rule", "");
      if (auto const payload = c.value("rulePayload", ""); !payload.empty()) {
        info.rule += "(" + payload + ")";
      }
      auto const& m = c["metadata"];
      info.network = m.value("network", "");
      // host is empty for connections by IP
      auto host = m.value("host", "");
      info.host = (host.empty() ? m.value("destinationIP", "") : host) + ":" + m.value("destinationPort", "");
      // chains go from the proxy to the group: show them the other way round
      auto chains = c.value("chains", std::vector<std::string>());
      std::reverse(chains.begin(), chains.end());
      for (auto const& link : chains) {
        info.chain += (info.chain.empty() ? "" : " > ") + link;
      }
      s.connections.push_back(std::move(info));
    }
    std::sort(s.connections.begin(), s.connections.end(),
        [](auto const& a, auto const& b) { return a.download > b.download; });
  }

  void traffic_loop() {
    auto http = http_client(opts_);
    while (!stop_) {
      http.stream(
          "/traffic",
          [&](std::string_view line) {
            auto const sample = json::parse(line, nullptr, false);
            if (!sample.is_object()) {
              return;
            }
            update([&](snapshot& s) {
              s.traffic.push_back({.up = sample.value("up", std::int64_t(0)), .down = sample.value("down", std::int64_t(0))});
              if (s.traffic.size() > traffic_history) {
                s.traffic.pop_front();
              }
            });
          },
          stop_);
      pause(1s); // stream broke (mihomo restarted?): reconnect
    }
  }

  void command_loop() {
    auto http = http_client(opts_);
    while (true) {
      auto cmd = command();
      {
        auto lock = std::unique_lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
        if (stop_) {
          return;
        }
        cmd = std::move(queue_.front());
        queue_.pop_front();
      }
      switch (cmd.type) {
      case command::kind::select: {
        auto const r = http.request("PUT", "/proxies/" + http.escape(cmd.first), json{{"name", cmd.second}}.dump());
        update([&](snapshot& s) {
          if (r.status == 204) {
            s.proxies[cmd.first].now = cmd.second; // show it right away, next poll confirms
          } else {
            s.error = "select: " + describe(r);
          }
        });
      } break;
      case command::kind::test_delay: {
        update([&](snapshot& s) { s.delay[cmd.first] = "testing"; });
        // mihomo itself fetches the url through the proxy and measures the time
        auto const r = http.request("GET",
            "/proxies/" + http.escape(cmd.first) + "/delay?timeout=3000&url=" +
                http.escape(opts_.delay_url),
            {}, 5s);
        auto const body = json::parse(r.body, nullptr, false);
        update([&](snapshot& s) {
          s.delay[cmd.first] = (r.status == 200 && body.contains("delay"))
                                   ? std::format("{} ms", body["delay"].get<int>())
                                   : (r.status == 504 ? "timeout" : "error");
        });
      } break;
      case command::kind::close_connection:
        http.request("DELETE", "/connections/" + http.escape(cmd.first));
        break;
      }
    }
  }

  options opts_;
  mutable std::mutex mutex_;
  snapshot state_;

  std::atomic<bool> stop_ = false;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::deque<command> queue_;

  std::thread poller_;
  std::thread traffic_;
  std::thread commands_;
};

// ---------------------------------------------------------------------------------------------
// UI

[[nodiscard]] auto human(std::int64_t bytes) -> std::string {
  constexpr auto units = std::to_array({"B", "KiB", "MiB", "GiB", "TiB"});
  auto value = double(bytes);
  auto unit = std::size_t(0);
  while (value >= 1024.0 && unit + 1 < units.size()) {
    value /= 1024.0;
    ++unit;
  }
  return unit == 0 ? std::format("{} B", bytes) : std::format("{:.1f} {}", value, units[unit]);
}

// one row of braille bars: newest sample on the right
void sparkline(std::deque<traffic_sample> const& traffic, std::int64_t traffic_sample::* field, int cells,
    xxx::im_color color) {
  auto const width = cells * 2;
  if (!xxx::canvas_begin(xxx::im_vec2(width, 4))) {
    return;
  }
  auto peak = std::int64_t(1);
  for (auto const& s : traffic) {
    peak = std::max(peak, s.*field);
  }
  auto const count = std::min<std::size_t>(traffic.size(), std::size_t(width));
  for (std::size_t i = 0; i < count; ++i) {
    auto const value = traffic[traffic.size() - count + i].*field;
    auto const h = value > 0 ? std::clamp(int((value * 4 + peak - 1) / peak), 1, 4) : 0;
    auto const x = width - int(count) + int(i);
    for (int y = 4 - h; y < 4; ++y) {
      xxx::canvas_point(xxx::im_vec2(x, y), color);
    }
  }
  xxx::canvas_end();
}

// widgets work only inside views: tabs live in the header view (ctrl-t)
void header(mihomo_api const& api, snapshot const& s, int& tab) {
  static constexpr auto tabs = std::to_array({"proxies"sv, "connections"sv});
  xxx::view_begin(std::format("mihomo {}##header", api.url()), xxx::im_view_flags_default, xxx::im_key_id::ctrl_t);
  if (s.connected) {
    auto const last = s.traffic.empty() ? traffic_sample() : s.traffic.back();
    xxx::push_color(xxx::im_color_id::text, 0x8ec07c_c);
    xxx::label(std::format("● {}", s.version));
    xxx::pop_color();
    xxx::same_line();
    xxx::label(std::format("  ↑ {}/s  ↓ {}/s   total ↑ {} ↓ {}   {} connections", human(last.up), human(last.down),
        human(s.upload_total), human(s.download_total), s.connections.size()));
  } else {
    xxx::push_color(xxx::im_color_id::text, 0xfb4934_c);
    xxx::label(std::format("✕ {}", s.error.empty() ? "connecting..." : s.error));
    xxx::pop_color();
  }
  if (s.connected && !s.error.empty()) {
    xxx::push_color(xxx::im_color_id::text, 0xfabd2f_c);
    xxx::label(s.error); // e.g. a failed command
    xxx::pop_color();
  }
  auto const cells = std::max(1, xxx::get_screen_rect().width() - 4);
  xxx::label("↓");
  xxx::same_line();
  sparkline(s.traffic, &traffic_sample::down, cells, 0x83a598_c);
  xxx::label("↑");
  xxx::same_line();
  sparkline(s.traffic, &traffic_sample::up, cells, 0xd3869b_c);
  xxx::tabs("tabs", tabs, tab);
  xxx::view_end();
}

struct ui_state {
  int tab = 0;
  int group = 0;
  int member = 0;
  int connection = 0;
  connection_info closing;
};

void proxies_tab(mihomo_api& api, snapshot const& s, ui_state& ui) {
  static constexpr auto group_columns = std::to_array<xxx::im_table_column>({
      {"group", xxx::fill()},
      {"type", xxx::cells(9)},
      {"selected", xxx::cells(12)},
  });
  static constexpr auto member_columns = std::to_array<xxx::im_table_column>({
      {"", xxx::cells(1)}, // selection marker
      {"proxy", xxx::fill()},
      {"type", xxx::cells(12)},
      {"delay", xxx::cells(9), xxx::im_align::right},
  });

  auto cells = std::vector<std::string>();
  for (auto const& name : s.groups) {
    auto const& g = s.proxies.at(name);
    cells.insert(cells.end(), {name, g.type, g.now});
  }

  xxx::layout_row_begin(2);
  xxx::layout_row_push(xxx::ratio(0.45f));
  xxx::view_begin("groups", xxx::im_view_flags_default, xxx::im_key_id::ctrl_g, xxx::fill(1));
  xxx::table("groups", group_columns, cells, ui.group);
  xxx::view_end();

  xxx::layout_row_push(xxx::fill());
  auto const group = (ui.group >= 0 && ui.group < int(s.groups.size())) ? s.groups[std::size_t(ui.group)] : "";
  xxx::view_begin(std::format("{}##members", group.empty() ? "members" : group), xxx::im_view_flags_default,
      xxx::im_key_id::ctrl_p, xxx::fill(1));
  if (!group.empty()) {
    auto const& g = s.proxies.at(group);
    if (xxx::button("test delays")) {
      for (auto const& name : g.all) {
        api.post({.type = command::kind::test_delay, .first = name, .second = {}});
      }
    }
    cells.clear();
    for (auto const& name : g.all) {
      auto const it = s.proxies.find(name);
      auto delay = std::string();
      if (auto const tested = s.delay.find(name); tested != s.delay.end()) {
        delay = tested->second;
      } else if (it != s.proxies.end() && it->second.last_delay > 0) {
        delay = std::format("{} ms", it->second.last_delay);
      }
      cells.insert(cells.end(), {name == g.now ? "●" : "", name, it != s.proxies.end() ? it->second.type : "", delay});
    }
    if (xxx::table("members", member_columns, cells, ui.member)) {
      api.post({.type = command::kind::select, .first = group, .second = g.all[std::size_t(ui.member)]});
    }
  }
  xxx::view_end();
  xxx::layout_row_end();
}

void connections_tab(snapshot const& s, ui_state& ui) {
  static constexpr auto columns = std::to_array<xxx::im_table_column>({
      {"host", xxx::fill()},
      {"net", xxx::cells(4)},
      {"chain", xxx::ratio(0.25f)},
      {"rule", xxx::ratio(0.15f)},
      {"↓", xxx::cells(10), xxx::im_align::right},
      {"↑", xxx::cells(10), xxx::im_align::right},
  });
  auto cells = std::vector<std::string>();
  for (auto const& c : s.connections) {
    cells.insert(cells.end(), {c.host, c.network, c.chain, c.rule, human(c.download), human(c.upload)});
  }
  xxx::view_begin(std::format("{} connections##connections", s.connections.size()), xxx::im_view_flags_default,
      xxx::im_key_id::ctrl_o, xxx::fill(1));
  if (xxx::table("connections", columns, cells, ui.connection) && ui.connection >= 0) {
    ui.closing = s.connections[std::size_t(ui.connection)];
    xxx::open_popup("close");
  }
  xxx::view_end();
}

} // namespace

int main(int argc, char** argv) {
  auto const opts = parse_options(argc, argv);
  if (!opts) {
    return opts.error();
  }
  ::curl_global_init(CURL_GLOBAL_DEFAULT);
  xxx::init();
  {
    // background threads start here and are joined before xxx::shutdown(): they call xxx::wake_up()
    auto api = mihomo_api(*opts);
    auto ui = ui_state();

    while (true) {
      // sleep until a key, a mouse event or new data from a background thread
      xxx::process_input_events(xxx::wait_forever);
      if (xxx::is_key_pressed(xxx::im_key_id::ctrl_q)) {
        break;
      }
      auto const s = api.copy();

      xxx::new_frame();
      header(api, s, ui.tab);
      if (ui.tab == 0) {
        proxies_tab(api, s, ui);
      } else {
        connections_tab(s, ui);
      }
      xxx::label(ui.tab == 0 ? "arrows move · Enter select · c-c copy row · c-q quit"
                             : "arrows move · Enter close connection · c-c copy row · c-q quit");

      if (xxx::popup_begin("close", "close connection?", 50)) {
        xxx::label(ui.closing.host);
        xxx::label(ui.closing.chain);
        xxx::label(" ");
        if (xxx::button("close")) {
          api.post({.type = command::kind::close_connection, .first = ui.closing.id, .second = {}});
          xxx::close_popup();
        }
        xxx::same_line();
        if (xxx::button("keep")) {
          xxx::close_popup();
        }
        xxx::popup_end();
      }
      xxx::render();
    }
  }
  xxx::shutdown();
  ::curl_global_cleanup();
  return 0;
}
