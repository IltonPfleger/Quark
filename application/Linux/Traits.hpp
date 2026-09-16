#pragma once

#include <Traits.hpp>

namespace QUARK {

class Application;

template <> struct Traits<Application> {
  static constexpr bool Virtualization = true;
  static constexpr bool Unprivileged = false;
};

template <> struct Traits<Deferred> {
  static constexpr size_t Threads = 1;
};

} // namespace QUARK
