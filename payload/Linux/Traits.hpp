#pragma once

#include <Traits.hpp>

namespace QUARK {

class Payload;

template <> struct Traits<Payload> {
  static constexpr bool Virtualization = true;
  static constexpr bool Unprivileged = false;
};

template <> struct Traits<Deferred> {
  static constexpr size_t Threads = 1;
};

} // namespace QUARK
