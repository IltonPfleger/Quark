#pragma once

#include <Traits.hpp>

namespace QUARK {

template <> struct Traits<Payload> {
    static constexpr unsigned long Size    = 128 * 1024;
    static constexpr unsigned long Address = Traits<MemoryMap>::RamStart + Size + 1 * 1024 * 1024;
    static constexpr bool Virtualized      = false;
};

} // namespace QUARK
