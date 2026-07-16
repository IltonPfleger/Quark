#pragma once

#include <Traits.hpp>

namespace QUARK {

class Payload;
template <> struct Traits<Payload> {
    static constexpr unsigned long Size    = 256 * 1024;
    static constexpr unsigned long Address = Traits<MemoryMap>::RamStart + Size * 10;
    static constexpr bool Virtualized      = true;
};

} // namespace QUARK
