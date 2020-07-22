#pragma once

#include <cassert>

#include "rxcpp/rx.hpp"

#include "wayland-server-core.h"

namespace rxwayland {
namespace detail {

template <class T>
struct signal_connector {
  wl_listener listener{};
  wl_listener destroy_listener{};
  bool receive_destroy_signal{false};
  rxcpp::subscriber<T> subscriber;

  explicit signal_connector(const rxcpp::subscriber<T>& s) : subscriber{s} {}
};

}  // namespace detail

template <class T>
rxcpp::observable<T> from_signal(wl_signal* signal, wl_signal* destroy_signal) {
  assert(signal != nullptr);
  assert(destroy_signal != nullptr);

  return rxcpp::observable<>::create<T>(
      [signal, destroy_signal](const rxcpp::subscriber<T>& subscriber) {
        using connector_t = detail::signal_connector<T>;
        auto conn = new connector_t{subscriber};

        conn->listener.notify = [](wl_listener* l, void* data) {
          connector_t* conn = wl_container_of(l, conn, listener);
          conn->subscriber.on_next(static_cast<T>(data));
        };
        conn->destroy_listener.notify = [](wl_listener* l, void*) {
          connector_t* conn = wl_container_of(l, conn, destroy_listener);
          conn->receive_destroy_signal = true;
          conn->subscriber.on_completed();
          delete conn;
        };

        conn->subscriber.add([conn]() {
          wl_list_remove(&conn->listener.link);
          wl_list_remove(&conn->destroy_listener.link);
          if (!conn->receive_destroy_signal) {
            delete conn;
          }
        });

        wl_signal_add(signal, &conn->listener);
        wl_signal_add(destroy_signal, &conn->destroy_listener);
      });
}

}  // namespace rxwayland