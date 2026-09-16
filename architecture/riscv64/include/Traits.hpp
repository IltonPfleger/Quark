#pragma once

#include <Traits.hpp>

namespace QUARK {

class RISCV;

template <> struct Traits<RISCV> {
  static constexpr bool Hypervisor = Traits<Application>::Virtualization;
  static constexpr bool Supervisor = Traits<Kernel>::Multitask;

  static_assert(!(Supervisor && Hypervisor));
};

} // namespace QUARK
