#pragma once

namespace QUARK::ABI {

typedef int Operation;
enum : Operation {
  READ,
  WRITE,

  // ABI_HEAP_NEW,
  // ABI_HEAP_DELETE,

  THREAD_CONSTRUCTOR,
  THREAD_DESTRUCTOR,
  THREAD_JOIN,
  EXIT,

  SEMAPHORE_CONSTRUCTOR,
  SEMAPHORE_DESTRUCTOR,
  SEMAPHORE_P,
  SEMAPHORE_V,
};

} // namespace QUARK::ABI
