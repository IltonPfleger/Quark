#pragma once

namespace QUARK {

template <> struct Traits<Payload> {
  static constexpr bool Virtualization = false;
  static constexpr bool Unprivileged = false;
};

} // namespace QUARK
