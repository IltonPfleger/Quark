#pragma once

#include <Traits.hpp>

namespace QUARK {

class RISCV;

template <> struct Traits<RISCV> {
  static constexpr bool Hypervisor = Traits<Application>::Virtualization;
  static constexpr bool Supervisor =
      Traits<Kernel>::Mode == Traits<Kernel>::KERNEL;
  static constexpr bool MMU = true;

  static_assert(!(Supervisor && Hypervisor));
};

} // namespace QUARK
