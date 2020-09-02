# RxWayland
RxWayland is an adaptor to allow that rxcpp run_loop schedules rx events on the wayland event loop.

# Example
```cpp
#include <chrono>

#include "rxwayland/console.hpp"
#include "rxwayland/run_loop.hpp"

int main() {
  auto loop = wl_event_loop_create();
  rxwayland::run_loop rl{loop};

  auto start = std::chrono::steady_clock::now() + std::chrono::microseconds(16);
  auto period = std::chrono::milliseconds(16);
  rxcpp::observable<>::interval(start, period)
      .take(60)
      .subscribe([](long v) { rxwayland::console::out("on_next(): {}\n", v); },
                 []() { rxwayland::console::out("on_completed()\n"); });

  rxwayland::console::out("run wayland event loop\n");
  while (wl_event_loop_dispatch(loop, -1) != -1) {
  }

  return 0;
}
```

# Building and Installing
```sh
mkdir build
cd build
cmake .. # (Optional) -DCMAKE_INSTALL_PREFIX=/opt/my-storage
```
# Usage with CMake
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(RXWAYLAND REQUIRED rxwayland)
target_include_directories(${PROJECT_NAME} PUBLIC ${RXWAYLAND_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${RXWAYLAND_LIBRARIES})
```

# Requirement
* [RxCpp](https://github.com/Reactive-Extensions/RxCpp)
* [Wayland](https://gitlab.freedesktop.org/wayland/wayland)