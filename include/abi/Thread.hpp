#pragma once

#include <Thread.hpp>
#include <Traits.hpp>
#include <abi/ABI.hpp>
#include <abi/Console.hpp>
#include <architecture/Syscall.hpp>

namespace QUARK::ABI {

class ThreadHandler {
public:
  ThreadHandler(void *(*function)(void *), void *argument) {
    function_ = function;
    argument_ = argument;
    handler_ = Syscall(ABI::THREAD_CONSTRUCTOR, _start, this);
  }

  ~ThreadHandler() { Syscall(ABI::THREAD_DESTRUCTOR, handler_); }

  void join() { Syscall(ABI::THREAD_JOIN, handler_); }

  static void _start(void *pointer) {
    ThreadHandler *self = reinterpret_cast<ThreadHandler *>(pointer);
    self->function_(self->argument_);
    exit();
  }

  static void exit() { Syscall(ABI::EXIT); }

private:
  void *(*function_)(void *);
  void *argument_;
  void *handler_;
};

using Thread = Meta::IF<Traits<Kernel>::Mode == Traits<Kernel>::KERNEL,
                        ThreadHandler, QUARK::Thread>::Result;

}; // namespace QUARK::ABI
