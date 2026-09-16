#pragma once

namespace QUARK {

template <> struct Traits<Application> {
  static constexpr bool Virtualization = false;
};

template <> struct Traits<Deferred> {
  static constexpr size_t Threads = 0;
};

} // namespace QUARK
