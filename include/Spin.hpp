#ifndef __QUARK_SPIN__
#define __QUARK_SPIN__

#include <architecture/CPU.hpp>
#include <types.hpp>
#include <utility/Guard.hpp>

namespace QUARK {

class Spin {
  public:
    constexpr Spin()
        : locked_(0) {}

    void acquire() {
        while (CPU::Atomic::tsl(locked_))
            ;
    }
    void release() { CPU::Atomic::store(locked_, 0); }

  public:
    using Guard = QUARK::Guard<Spin, &Spin::acquire, &Spin::release>;

  private:
    volatile uint32_t locked_;
};

} // namespace QUARK

#endif
