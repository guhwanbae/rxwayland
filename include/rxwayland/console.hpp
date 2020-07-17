#pragma once

#include <fmt/format.h>

#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

namespace rxwayland {

namespace detail {

inline std::string get_tid() {
  std::stringstream ss{};
  ss << std::setw(16) << std::this_thread::get_id();
  return ss.str();
}

inline std::string get_timestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto now_as_time_t = std::chrono::system_clock::to_time_t(now);
  const auto fractional_millisec =
      (std::chrono::duration_cast<std::chrono::milliseconds>(
           now.time_since_epoch()) %
       1000)
          .count();

  std::stringstream ss{};
  ss << std::put_time(std::localtime(&now_as_time_t), "%T") << "."
     << std::setfill('0') << std::setw(3) << fractional_millisec;
  return ss.str();
}

}  // namespace detail

class console {
 public:
  using clock_type = std::chrono::steady_clock;

  console(const console&) = delete;
  console& operator=(const console&) = delete;

  console(console&&) = delete;
  console& operator=(console&&) = delete;

  template <class... Args>
  static void out(Args&&... args) {
    const thread_local auto tid = detail::get_tid();

    auto msg = fmt::format(std::forward<Args>(args)...);
    auto timestamp = detail::get_timestamp();

    std::lock_guard<std::mutex> lock{instance().mutex_};
    fmt::print("[{} {:10} ms]{}", tid, std::move(timestamp), std::move(msg));
  }

 private:
  console() = default;
  ~console() = default;

  static console& instance() {
    static console instance{};
    return instance;
  }

  std::mutex mutex_;
};

}  // namespace rxwayland