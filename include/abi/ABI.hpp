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

  // ABI_SEMAPHORE_CONSTRUCTOR,
  // ABI_SEMAPHORE_DESTRUCTOR,
  // ABI_SEMAPHORE_P,
  // ABI_SEMAPHORE_V,
};

} // namespace QUARK::ABI
