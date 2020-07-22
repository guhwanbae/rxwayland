#include "rxwayland/console.hpp"
#include "rxwayland/run_loop.hpp"
#include "rxwayland/signal.hpp"

#include "wayland-server.h"

struct compositor {
  wl_event_loop* loop;
  wl_signal interval_signal;
  wl_signal destroy_signal;

  wl_event_source* interval_source;
  wl_event_source* destroy_source;

  compositor() {
    loop = wl_event_loop_create();
    wl_signal_init(&interval_signal);
    wl_signal_init(&destroy_signal);

    interval_source = wl_event_loop_add_timer(
        loop,
        [](void* data) -> int {
          auto this_ptr = static_cast<compositor*>(data);
          wl_signal_emit(&this_ptr->interval_signal, this_ptr);
          wl_event_source_timer_update(this_ptr->interval_source, 100);
          return 1;
        },
        this);
    wl_event_source_timer_update(interval_source, 100);

    destroy_source = wl_event_loop_add_timer(
        loop,
        [](void* data) -> int {
          auto this_ptr = static_cast<compositor*>(data);
          wl_signal_emit(&this_ptr->destroy_signal, this_ptr);
          wl_event_source_remove(this_ptr->interval_source);
          wl_event_source_remove(this_ptr->destroy_source);
          wl_event_loop_destroy(this_ptr->loop);
          return 0;
        },
        this);
    wl_event_source_timer_update(destroy_source, 550);
  }

  void run_loop() {
    while (wl_event_loop_dispatch(loop, -1) != -1) {
    }
  }
};

int main() {
  rxwayland::console::out("main\n");
  compositor comp{};
  rxwayland::run_loop rl{comp.loop};

  auto source = rxwayland::from_signal<compositor*>(&comp.interval_signal,
                                                    &comp.destroy_signal)
                    .map([count = 0l](compositor*) mutable { return count++; })
                    .publish()
                    .ref_count();
  auto s1 = source.subscribe(
      [](long n) { rxwayland::console::out("1-on_next(): {}\n", n); },
      []() { rxwayland::console::out("1-on_completed()\n"); });
  auto s2 = source.subscribe(
      [](long n) { rxwayland::console::out("2-on_next(): {}\n", n); },
      []() { rxwayland::console::out("2-on_completed()\n"); });

  rxwayland::console::out("run wayland event loop\n");
  comp.run_loop();

  rxwayland::console::out("exit\n");
  return 0;
}