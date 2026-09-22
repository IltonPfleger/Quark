#ifndef __QUARK_SYNCHRONIZATION_SEMAPHORE__
#define __QUARK_SYNCHRONIZATION_SEMAPHORE__

#include <Thread.hpp>
#include <synchronization/Spin.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

class Semaphore : Spin {
public:
  constexpr Semaphore(uint32_t value = 0) : value_(value), waiting_() {}

  ~Semaphore() { assert(value_ >= 0); }

  void p() {
    CPU::IRQ::Guard _;

    this->acquire();

    if (CPU::Atomic::fdec(value_) <= 0) {
      Thread::sleep(&waiting_, this);
    } else {
      this->release();
    }
  }

  void v() {
    CPU::IRQ::Guard _;
    this->acquire();
    if (CPU::Atomic::finc(value_) < 0)
      Thread::wakeup(&waiting_);
    this->release();
  }

protected:
  int value_;
  Thread::List waiting_;
};

} // namespace QUARK

#endif
