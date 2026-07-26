#pragma once

#include <Traits.hpp>

namespace QUARK {

class RISCV;

template <> struct Traits<RISCV> {
    static constexpr bool User       = Traits<Kernel>::Privileged;
    static constexpr bool Supervisor = false;
    static constexpr bool Hypervisor = Traits<Payload>::Virtualized;
    static constexpr bool FPU        = true;
    static_assert(!(Supervisor && Hypervisor));
};

} // namespace QUARK
