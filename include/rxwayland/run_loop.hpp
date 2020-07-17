#pragma once

#include <unistd.h>

#include <cassert>
#include <thread>

#include "rxcpp/rx.hpp"
#include "timer.hpp"
#include "wayland-server.h"

namespace rxwayland {

class run_loop {
 public:
  using clock_type = rxcpp::schedulers::run_loop::clock_type;

  explicit run_loop(wl_event_loop* loop)
      : rx_run_loop_{},
        loop_{loop},
        loop_owner_tid_{std::this_thread::get_id()},
        timer_{},
        timer_source_{nullptr} {
    assert(loop_ != nullptr);

    // For sake of wayland compositor's simplicity and lightness,
    // libwayland-server is designed with single threaded model.
    // Note that user must call libwayland-server facilities on the thread which
    // run wayland event loop context.
    timer_source_ = wl_event_loop_add_fd(
        loop, timer_.get_fd(), WL_EVENT_READABLE,
        [](int fd, uint32_t, void* data) -> int {
          // Discard timer data.
          uint64_t val{};
          read(fd, &val, sizeof(uint64_t));

          // Dispatch scheduled rx events.
          auto this_ptr = static_cast<run_loop*>(data);
          this_ptr->on_rx_event_scheduled();
          return 0;
        },
        this);

    // Let rxcpp run_loop reschedule rx events on wayland event loop.
    rx_run_loop_.set_notify_earlier_wakeup(
        [this](const clock_type::time_point& wakeup_point) {
          this->on_earlier_wakeup(wakeup_point);
        });
  }

  ~run_loop() {
    rx_run_loop_.set_notify_earlier_wakeup({});

    remove_timer_source();
  }

  run_loop(const run_loop&) = delete;
  run_loop& operator=(const run_loop&) = delete;

  run_loop(run_loop&&) = delete;
  run_loop& operator=(run_loop&&) = delete;

  inline const rxcpp::schedulers::run_loop& get_rx_run_loop() const noexcept {
    return rx_run_loop_;
  }

  void remove_timer_source() {
    assert(loop_owner_tid_ == std::this_thread::get_id());

    // Note that wayland event source must be removed before destroying wayland
    // event loop.
    if (timer_source_ != nullptr) {
      wl_event_source_remove(timer_source_);
      timer_source_ = nullptr;
    }
  }

 private:
  void on_earlier_wakeup(const clock_type::time_point& wakeup_point) {
    const auto now = clock_type::now();
    const auto last_timeout_point = timer_.get_timeout_point();

    if (wakeup_point < last_timeout_point || last_timeout_point < now) {
      timer_.set_timeout_point(wakeup_point);
    }
  }

  void on_rx_event_scheduled() {
    assert(loop_owner_tid_ == std::this_thread::get_id());

    while (!rx_run_loop_.empty() &&
           rx_run_loop_.peek().when < rx_run_loop_.now()) {
      rx_run_loop_.dispatch();
    }

    if (!rx_run_loop_.empty()) {
      timer_.set_timeout_point(rx_run_loop_.peek().when);
    }
  }

  rxcpp::schedulers::run_loop rx_run_loop_;

  wl_event_loop* loop_;
  std::thread::id loop_owner_tid_;

  timer<clock_type> timer_;
  wl_event_source* timer_source_;
};

}  // namespace rxwayland