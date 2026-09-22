#pragma once

#include <Traits.hpp>

namespace QUARK {

template <> struct Traits<Payload> {
    static constexpr unsigned long Address = Traits<MemoryMap>::RamStart + 128 * 1024;
    static constexpr size_t Size           = 128 * 1024;
    static constexpr bool Virtualized      = false;
};

} // namespace QUARK
