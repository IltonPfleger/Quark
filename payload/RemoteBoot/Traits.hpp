#pragma once

#include <Traits.hpp>

namespace QUARK {

class Payload;
template <> struct Traits<Payload> {
    static constexpr size_t Size           = 1024 * 128;
    static constexpr unsigned long Address = Traits<MemoryMap>::RamStart + Size * 2;
    static constexpr bool Virtualized      = false;
};

} // namespace QUARK
