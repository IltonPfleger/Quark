#pragma once

#include <Traits.hpp>

namespace QUARK {

template <> struct Traits<Payload> {
    static constexpr uintptr_t Address   = Traits<MemoryMap>::RamStart + 1 * 1024 * 1024;
    static constexpr bool Virtualization = false;
    static constexpr bool Unprivileged   = false;
};

template <> struct Traits<Deferred> {
    static constexpr size_t Threads = 0;
    static constexpr bool Enable    = Threads > 0;
};

} // namespace QUARK
