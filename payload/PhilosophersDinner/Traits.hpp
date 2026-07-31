#pragma once

#include <Traits.hpp>

namespace QUARK {

template <> struct Traits<Payload> {
    static constexpr uintptr_t Address   = Traits<MemoryMap>::RamStart + 1 * 1024 * 1024;
    static constexpr bool Virtualization = false;
    static constexpr bool Unprivileged   = false;
};

} // namespace QUARK
