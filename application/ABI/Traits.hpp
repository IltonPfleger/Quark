#pragma once

#include <Traits.hpp>

namespace QUARK {

template <> struct Traits<Payload> {
    static constexpr bool Virtualization = false;
    static constexpr bool Unprivileged   = true;
};

template <> struct Traits<Deferred> {
    static constexpr size_t Threads = 0;
};

} // namespace QUARK
