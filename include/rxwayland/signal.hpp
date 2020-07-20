#pragma once

#include <cassert>
#include <memory>

#include "rxcpp/rx.hpp"

#include "wayland-server-core.h"

namespace rxwayland {
namespace detail {

template <class T>
struct listener_with_subscriber {
  using this_t = listener_with_subscriber<T>;

  wl_listener listener;
  wl_listener destroy_listener;
  rxcpp::subscriber<T> subscriber;

  listener_with_subscriber(const rxcpp::subscriber<T>& s)
      : listener{}, destroy_listener{}, subscriber{s} {
    listener.notify = [](wl_listener* l, void* data) {
      this_t* this_ptr = wl_container_of(l, this_ptr, listener);
      this_ptr->subscriber.on_next(static_cast<T>(data));
    };
    destroy_listener.notify = [](wl_listener* l, void*) {
      this_t* this_ptr = wl_container_of(l, this_ptr, destroy_listener);
      this_ptr->subscriber.on_completed();
    };
  }
};

}  // namespace detail

template <class T>
rxcpp::observable<T> from_signal(wl_signal* signal, wl_signal* destroy_signal) {
  assert(signal != nullptr);
  assert(destroy_signal != nullptr);

  return rxcpp::observable<>::create<T>(
      [signal, destroy_signal](const rxcpp::subscriber<T>& subscriber) {
        auto lws =
            std::make_shared<detail::listener_with_subscriber<T>>(subscriber);

        lws->subscriber.add([lws]() {
          wl_list_remove(&(lws->listener.link));
          wl_list_remove(&(lws->destroy_listener.link));
        });

        wl_signal_add(signal, &(lws->listener));
        wl_signal_add(destroy_signal, &(lws->destroy_listener));
      });
}

}  // namespace rxwayland