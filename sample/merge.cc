#include <chrono>

#include "console.hpp"
#include "rxcpp/rx.hpp"
#include "rxwayland.hpp"

int main() {
  rxwayland::console::out("main\n");

  auto loop = wl_event_loop_create();
  rxwayland::run_loop rl{loop};

  auto main_thread = rxcpp::observe_on_run_loop(rl.get_rx_run_loop());
  auto worker_threads = rxcpp::synchronize_new_thread();

  auto source1 = rxcpp::observable<>::interval(std::chrono::milliseconds(10))
                     .subscribe_on(worker_threads)
                     .map([](auto n) {
                       rxwayland::console::out("1-map: {} -> {}\n", n, n * 2);
                       return n * 2;
                     });

  auto source2 = rxcpp::observable<>::interval(std::chrono::milliseconds(20))
                     .subscribe_on(worker_threads)
                     .map([](auto n) {
                       rxwayland::console::out("2-map: {} -> {}\n", n, -n);
                       return -n;
                     });

  source1.merge(source2)
      .take(10)
      .observe_on(main_thread)
      .subscribe([](auto n) { rxwayland::console::out("on_next: {}\n", n); },
                 [&]() {
                   rxwayland::console::out("on_complete\n");
                   rxwayland::console::out("quit glib event loop\n");

                   // Note that wayland server functions must be called on the
                   // thread which runs waylnad event loop context.
                   rl.remove_timer_source();
                   wl_event_loop_destroy(loop);
                 });

  rxwayland::console::out("run wayland event loop\n");
  while (wl_event_loop_dispatch(loop, -1) == 0) {
  }

  return 0;
}