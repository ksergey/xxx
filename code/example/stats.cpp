// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

#include <xxx/ui.h>

template <class Range1, class Range2, class Fn2>
inline void for_each(Range1&& range1, Range2&& range2, Fn2&& fn2) {
  using std::begin;
  using std::end;

  auto first1 = begin(range1);
  auto last1 = end(range1);
  auto first2 = begin(range2);
  auto last2 = end(range2);

  for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
    fn2(*first1, *first2);
  }
}

struct cpu_usage {
  std::string_view name;
  float value;
};

class cpu_monitor {
 private:
  using clock = std::chrono::steady_clock;

  struct cpu_stats {
    std::string name;
    std::uint64_t total;
    std::uint64_t work;
  };

  static constexpr std::chrono::seconds interval_{1};

  std::vector<cpu_stats> stats_[2];
  std::size_t current_stat_index_{0};
  clock::time_point timestamp_;
  std::vector<cpu_usage> usage_;

 public:
  cpu_monitor() {
    read_proc_stats();
    timestamp_ = clock::now();
  }

  bool update() {
    auto now = clock::now();
    if (timestamp_ + interval_ > now) {
      return false;
    }

    swap();
    read_proc_stats();
    timestamp_ = now;

    usage_.clear();

    ::for_each(current(), prev(), [this](cpu_stats const& current, cpu_stats const& prev) {
      auto work_over_period = static_cast<double>(current.work - prev.work);
      auto total_over_period = static_cast<double>(current.total - prev.total);
      auto& entry = usage_.emplace_back();
      entry.name = current.name;
      entry.value = (work_over_period * 100.0) / total_over_period;
    });

    return true;
  }

  std::vector<cpu_usage> const& usage() const noexcept { return usage_; }

 private:
  // Return current cpu_stats buffer.
  std::vector<cpu_stats>& current() noexcept { return stats_[current_stat_index_]; }

  // Return prev cpu_stats buffer.
  std::vector<cpu_stats> const& prev() const noexcept { return stats_[(current_stat_index_ + 1) % 2]; }

  // Swap current buffer with prev.
  void swap() noexcept { current_stat_index_ = (current_stat_index_ + 1) % 2; }

  // Read /proc/stat into current cpu_stats buffer.
  void read_proc_stats() {
    constexpr std::string_view begin_cpu{"cpu"};

    auto& out = current();
    out.clear();

    std::ifstream is{"/proc/stat"};

    std::string line;
    while (std::getline(is, line)) {
      if (line.compare(0, begin_cpu.size(), begin_cpu) != 0) {
        break;
      }
      std::istringstream ss{line};

      auto& entry = out.emplace_back();
      entry.total = 0;
      entry.work = 0;

      ss >> entry.name;

      std::uint64_t value;
      for (std::size_t i = 0; i < 3; ++i) {
        ss >> value;
        entry.work += value;
        entry.total += value;
      }

      while (ss >> value) {
        entry.total += value;
      }
    }
  }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  try {
    bool running = true;
    float spinner_step_storage{0};

    xxx::init();

    cpu_monitor monitor;

    while (running) {
      xxx::update(100);
      monitor.update();

      running = !xxx::is_key_pressed(xxx::key::esc);

      xxx::begin();
      xxx::row_begin(1);
      xxx::row_push(30);
      xxx::panel_begin("CPU");
      if (monitor.usage().empty()) {
        xxx::spinner(spinner_step_storage, "Loading");
      } else {
        for (auto const& cpu : monitor.usage()) {
          float value = cpu.value;
          xxx::row_begin(2);
          xxx::row_push(5);
          xxx::label(cpu.name);
          xxx::row_push(0.9);
          xxx::progress(value);
          xxx::row_end();
        }
      }
      xxx::panel_end();
      xxx::row_end();
      xxx::end();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::cout << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
