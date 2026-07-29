#pragma once

#include <Traits.hpp>

namespace QUARK {

class RISCV;

template <> struct Traits<RISCV> {
    static constexpr bool User       = Traits<Payload>::Unprivileged;
    static constexpr bool Hypervisor = Traits<Payload>::Virtualization;
    static constexpr bool Supervisor = false;
    static_assert(!(Supervisor && Hypervisor));
};

} // namespace QUARK
