#pragma once

#include <Traits.hpp>

namespace QUARK {

template <> struct Traits<Payload> {
    static constexpr size_t Size         = 1 * 1024 * 1024;
    static constexpr uintptr_t Address   = Traits<MemoryMap>::RamStart + Size * 4;
    static constexpr bool Virtualization = false;
    static constexpr bool Unprivileged   = false;
};

} // namespace QUARK
