#pragma once

namespace QUARK {

template <> struct Traits<Payload> {
  static constexpr bool Virtualization = false;
  static constexpr bool Unprivileged = false;
};

template <> struct Traits<Deferred> {
  static constexpr bool Enable = true;
  static constexpr size_t Threads = 1;
};

} // namespace QUARK
