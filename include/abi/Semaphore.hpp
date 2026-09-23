#pragma once

#include <abi/ABI.hpp>
#include <architecture/Syscall.hpp>
#include <synchronization/Semaphore.hpp>

namespace QUARK::ABI {

class SemaphoreHandler {
public:
  SemaphoreHandler(int value = 0) {
    handler_ = Syscall(ABI::SEMAPHORE_CONSTRUCTOR, value);
  }

  ~SemaphoreHandler() { Syscall(ABI::SEMAPHORE_DESTRUCTOR, handler_); }

  void p() { Syscall(ABI::SEMAPHORE_P, handler_); }
  void v() { Syscall(ABI::SEMAPHORE_V, handler_); }

private:
  void *handler_;
};

using Semaphore = Meta::IF<Traits<Kernel>::Mode == Traits<Kernel>::KERNEL,
                           SemaphoreHandler, QUARK::Semaphore>::Result;

}; // namespace QUARK::ABI
