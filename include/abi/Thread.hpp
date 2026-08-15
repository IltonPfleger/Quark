#pragma once

#include <abi/ABI.hpp>
#include <architecture/Syscall.hpp>

namespace QUARK::ABI {

class Thread {
public:
  template <typename Function, typename Argment> Thread(Function f, Argment a) {
    handler_ = Syscall(ABI::Function::ABI_THREAD_CONSTRUCTOR, f, a);
  }

  ~Thread() { Syscall(ABI::Function::ABI_THREAD_DESTRUCTOR, handler_); }

  void join() { Syscall(ABI::Function::ABI_THREAD_JOIN, handler_); }

  static void exit() { Syscall(ABI::Function::ABI_THREAD_EXIT); }

private:
  void *handler_;
};

}; // namespace QUARK::ABI
