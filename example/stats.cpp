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

#include <xxx.h>

template<class Range1, class Range2, class Fn2>
inline void forEach(Range1&& range1, Range2&& range2, Fn2&& fn2) {
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

struct CPUUsage {
  std::string_view name;
  float value;
};

class CPUMonitor {
private:
  using Clock = std::chrono::steady_clock;

  struct CPUStats {
    std::string name;
    std::uint64_t total;
    std::uint64_t work;
  };

  static constexpr std::chrono::seconds interval_{1};

  std::vector<CPUStats> stats_[2];
  std::size_t currentStatIndex_{0};
  Clock::time_point timestamp_;
  std::vector<CPUUsage> usage_;

public:
  CPUMonitor() {
    readProcStats();
    timestamp_ = Clock::now();
  }

  bool update() {
    auto now = Clock::now();
    if (timestamp_ + interval_ > now) {
      return false;
    }

    swap();
    readProcStats();
    timestamp_ = now;

    usage_.clear();

    forEach(current(), prev(), [this](CPUStats const& current, CPUStats const& prev) {
      auto workOverPeriod = static_cast<double>(current.work - prev.work);
      auto totalOverPeriod = static_cast<double>(current.total - prev.total);
      auto& entry = usage_.emplace_back();
      entry.name = current.name;
      entry.value = (workOverPeriod * 100.0) / totalOverPeriod;
    });

    return true;
  }

  std::vector<CPUUsage> const& usage() const noexcept {
    return usage_;
  }

private:
  // Return current cpu stats buffer.
  std::vector<CPUStats>& current() noexcept {
    return stats_[currentStatIndex_];
  }

  // Return prev cpu_stats buffer.
  std::vector<CPUStats> const& prev() const noexcept {
    return stats_[(currentStatIndex_ + 1) % 2];
  }

  // Swap current buffer with prev.
  void swap() noexcept {
    currentStatIndex_ = (currentStatIndex_ + 1) % 2;
  }

  // Read /proc/stat into current cpu stats buffer.
  void readProcStats() {
    constexpr std::string_view beginCpu("cpu");

    auto& out = current();
    out.clear();

    std::ifstream is("/proc/stat");

    std::string line;
    while (std::getline(is, line)) {
      if (line.compare(0, beginCpu.size(), beginCpu) != 0) {
        break;
      }
      std::istringstream ss(line);

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

    xxx::init();

    CPUMonitor monitor;

    while (running) {
      xxx::update(100);
      monitor.update();

      running = !xxx::isKeyPressed(xxx::key::Esc);

      xxx::begin();
      xxx::rowBegin(1);
      xxx::rowPush(30);
      xxx::panelBegin("CPU");
      if (monitor.usage().empty()) {
        xxx::spinner("Loading");
      } else {
        for (auto const& cpu : monitor.usage()) {
          double value = cpu.value;
          xxx::rowBegin(2);
          xxx::rowPush(5);
          xxx::label(cpu.name);
          xxx::rowPush(0.9);
          xxx::progress(value);
          xxx::rowEnd();
        }
      }
      xxx::panelEnd();
      xxx::rowEnd();
      xxx::end();
    }

    xxx::shutdown();
  } catch (std::exception const& e) {
    std::cout << "ERROR: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
