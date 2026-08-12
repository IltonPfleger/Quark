#pragma once

#include <Traits.hpp>

namespace QUARK {

template <> struct Traits<Payload> {
    static constexpr size_t Size         = 1 * 1024 * 1024;
    static constexpr uintptr_t Address   = Traits<MemoryMap>::RamStart + 4 * Size;
    static constexpr bool Virtualization = false;
    static constexpr bool Unprivileged   = true;
};

template <> struct Traits<Deferred> {
    static constexpr size_t Threads = 0;
};

} // namespace QUARK
