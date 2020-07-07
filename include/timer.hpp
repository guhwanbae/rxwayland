#pragma once

#include <sys/timerfd.h>
#include <unistd.h>

#include <chrono>
#include <mutex>
#include <ratio>
#include <stdexcept>

namespace rxwayland {

template <class clock_type>
class timer {
 public:
  timer()
      : fd_{timerfd_create(CLOCK_MONOTONIC, 0)},
        mutex_{},
        is_running_{false},
        timeout_point_{clock_type::time_point::min()} {
    if (fd_ == -1) {
      throw std::runtime_error{"rxwayland::timer: timerfd_create() failed."};
    }
  }

  ~timer() {
    if (fd_ != 0) {
      if (is_running_) {
        stop();
      }
      close(fd_);
    }
  }

  timer(const timer&) = delete;
  timer& operator=(const timer&) = delete;

  timer(timer&&) = delete;
  timer& operator=(timer&&) = delete;

  inline int get_fd() const noexcept { return fd_; }

  inline bool is_running() const noexcept { return is_running_; }

  void set_timeout_point(const typename clock_type::time_point& timeout_point) {
    const auto now = clock_type::now();
    const auto nanosec = static_cast<int>(
        std::max(std::chrono::duration_cast<std::chrono::nanoseconds>(
                     timeout_point - now),
                 std::chrono::nanoseconds::zero())
            .count());

    const int sec = nanosec / std::nano::den;
    const int fraction_nanosec =
        std::max(static_cast<std::intmax_t>(1), nanosec % std::nano::den);

    itimerspec disarm_period{};
    itimerspec period{{0, 0}, {sec, fraction_nanosec}};

    std::lock_guard<std::mutex> lock{mutex_};
    // Disarm timer.
    if (timerfd_settime(fd_, 0, &disarm_period, nullptr) == -1) {
      throw std::runtime_error{"rxwayland::timer: timerfd_settime() failed."};
    }

    // Reset timer.
    if (timerfd_settime(fd_, 0, &period, nullptr) == -1) {
      throw std::runtime_error{"rxwayland::timer: timerfd_settime() failed."};
    }

    is_running_ = true;
    timeout_point_ = timeout_point;
  }

  void stop() {
    itimerspec disarm_period{};

    std::lock_guard<std::mutex> lock{mutex_};
    if (timerfd_settime(fd_, 0, &disarm_period, nullptr) == -1) {
      throw std::runtime_error{"rxwayland::timer: failed to disarm."};
    }

    is_running_ = false;
  }

  typename clock_type::time_point get_timeout_point() const noexcept {
    std::lock_guard<std::mutex> lock{mutex_};
    return timeout_point_;
  }

 private:
  const int fd_;
  mutable std::mutex mutex_;
  bool is_running_;
  typename clock_type::time_point timeout_point_;
};

}  // namespace rxwayland