#pragma once

#include <Traits.hpp>

namespace QUARK {

class Payload;
template <> struct Traits<Payload> {
    static constexpr size_t Size           = 128 * 1024 * 1024;
    static constexpr unsigned long Address = Traits<MemoryMap>::RamStart + 2 * Size;
    static constexpr bool Virtualized      = true;
};

} // namespace QUARK
