#pragma once

#include <Semaphore.hpp>
#include <utility/Guard.hpp>

namespace QUARK {

class Mutex : Semaphore {
  public:
    constexpr Mutex()
        : Semaphore(1) {}

    void acquire() { p(); }
    void release() { v(); }

    using Guard = QUARK::Guard<Mutex, &Mutex::acquire, &Mutex::release>;
};

} // namespace QUARK
