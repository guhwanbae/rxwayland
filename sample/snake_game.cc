#include <chrono>
#include <cstdio>
#include <utility>
#include <variant>

#include <rxcpp/rx.hpp>
#include <rxwayland/run_loop.hpp>

using namespace std;
using namespace std::chrono;
using namespace rxcpp;
using namespace rxcpp::sources;

struct Snake {
  pair<int, int> position = {0, 0};

  void moveLeft() { --position.first; }
  void moveRight() { ++position.first; }
  void moveUp() { --position.second; }
  void moveDown() { ++position.second; }
};

struct LeftKey {};
struct RightKey {};
struct UpKey {};
struct DownKey {};
using Key = variant<LeftKey, RightKey, UpKey, DownKey>;

Key getLeftKey(const long) { return LeftKey{}; }
Key getRightKey(const long) { return RightKey{}; }
Key getUptKey(const long) { return UpKey{}; }
Key getDownKey(const long) { return DownKey{}; }

// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

Snake handleKeyEvent(const Key& key, Snake model) {
  visit(overloaded{
            [&](LeftKey) {
              printf("[input  ]Left  arrow key: move left\n");
              model.moveLeft();
            },
            [&](RightKey) {
              printf("[input  ]Right arrow key: move right\n");
              model.moveRight();
            },
            [&](UpKey) {
              printf("[input  ]Up    arrow key: move up\n");
              model.moveUp();
            },
            [&](DownKey) {
              printf("[input  ]Down  arrow key: move down\n");
              model.moveDown();
            },
        },
        key);
  return model;
}

void drawModel(const Snake& model) {
  const auto [x, y] = model.position;
  printf("[display]Draw Snake(%d,%d)\n", x, y);
}

int main() {
  auto loop = wl_event_loop_create();
  {
    rxwayland::run_loop rl{loop};
    composite_subscription cs{};

    printf("[system ]Make a Model\n");
    const Snake initModel{};
    subjects::behavior<Snake> bSnake{initModel};
    auto sSnake = bSnake.get_observable();

    const auto beginPoint = steady_clock::now() + seconds(1);
    printf("[system ]Build a KeyBoard Event Stream\n");
    auto sKeyLeft = never<Key>();
    auto sKeyRight = interval(beginPoint, seconds(1)).map(getRightKey).take(5);
    auto sKeyUp = never<Key>();
    auto sKeyDown = interval(beginPoint, seconds(2)).map(getDownKey).take(3);
    auto sKeyEvent = sKeyLeft.merge(sKeyRight, sKeyUp, sKeyDown);
    auto o = sKeyEvent.with_latest_from(handleKeyEvent, sSnake)
                 .subscribe(bSnake.get_subscriber());
    cs.add(o);

    printf("[system ]Build a Game Display Stream\n");
    o = sSnake.subscribe(drawModel);
    cs.add(o);

    printf("[system ]Run Event Loop\n");
    while (-1 != wl_event_loop_dispatch(loop, -1)) {
    }

    cs.unsubscribe();
  }
  wl_event_loop_destroy(loop);

  return 0;
}
