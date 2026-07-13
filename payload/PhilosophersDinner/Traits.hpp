#pragma once

#include <Traits.hpp>

namespace QUARK {

template <> struct Traits<Payload> {
    static constexpr size_t Size       = 256 * 1024;
    static constexpr uintptr_t Address = Traits<MemoryMap>::RamStart + Size * 2;
    static constexpr bool Virtualized  = false;
};

} // namespace QUARK
