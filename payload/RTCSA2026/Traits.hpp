#pragma once

#include <Traits.hpp>

namespace QUARK {

class Payload;
template <> struct Traits<Payload> {
    static constexpr unsigned long Address = Traits<MemoryMap>::RamStart + 1024 * 1024 * 32;
    static constexpr bool Virtualization   = true;
    static constexpr bool Unprivileged     = false;
};

template <> struct Traits<Deferred> {
    static constexpr bool Enable    = true;
    static constexpr size_t Threads = 1;
};

} // namespace QUARK
