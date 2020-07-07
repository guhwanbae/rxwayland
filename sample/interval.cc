#include <chrono>

#include "console.hpp"
#include "rxcpp/rx.hpp"
#include "rxwayland.hpp"

int main() {
  rxwayland::console::out("main\n");

  auto loop = wl_event_loop_create();
  rxwayland::run_loop rl{loop};

  auto source =
      rxcpp::observable<>::interval(std::chrono::milliseconds(10))
          .take(10)
          .filter([](auto n) {
            rxwayland::console::out("filter: {} % 2 = {}\n", n, n % 2);
            return n % 2 == 0;
          })
          .map([](auto n) {
            rxwayland::console::out("map: {} -> {}\n", n, n * 2);
            return n * 2;
          });

  source.subscribe([](auto n) { rxwayland::console::out("on_next: {}\n", n); },
                   [&]() {
                     rxwayland::console::out("on_complete\n");
                     rxwayland::console::out("quit glib event loop\n");
                     rl.remove_timer_source();
                     wl_event_loop_destroy(loop);
                   });

  rxwayland::console::out("run wayland event loop\n");
  while (wl_event_loop_dispatch(loop, -1) == 0) {
  }

  return 0;
}