#ifndef __QUARK_SYNCHRONIZATION_MUTEX__
#define __QUARK_SYNCHRONIZATION_MUTEX__

#include <synchronization/Semaphore.hpp>
#include <utility/Guard.hpp>

namespace QUARK {

class Mutex : Semaphore {
public:
  constexpr Mutex() : Semaphore(1) {}
  constexpr ~Mutex() {}

  void acquire() { p(); }

  void release() { v(); }

  using Guard = QUARK::Guard<Mutex, &Mutex::acquire, &Mutex::release>;
};

} // namespace QUARK

#endif
