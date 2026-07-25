#pragma once

#include <Spin.hpp>
#include <Thread.hpp>

namespace QUARK {

class Semaphore {
  public:
    constexpr Semaphore(int value = 0)
        : value_(value),
          waiting_(),
          lock_() {}

    void p() {
        CPU::IRQ::Guard _;

        lock_.acquire();

        if (CPU::Atomic::fdec(value_) < 1) {
            Thread::sleep(&waiting_, &lock_);
        } else {
            lock_.release();
        }
    }

    void v() {
        CPU::IRQ::Guard _;
        lock_.acquire();
        if (CPU::Atomic::finc(value_) < 0) Thread::wakeup(&waiting_);
        lock_.release();
    }

  private:
    volatile int value_;
    Thread::List waiting_;
    Spin lock_;
};

} // namespace QUARK
