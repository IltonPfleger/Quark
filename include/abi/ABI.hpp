#ifndef __QUARK_ABI_ABI__
#define __QUARK_ABI_ABI__

#include <types.hpp>

namespace QUARK::ABI {

using namespace QUARK::Types;
namespace Meta = QUARK::Meta;

typedef int Operation;
enum : Operation {
  READ,
  WRITE,

  ALLOC,
  FREE,

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

#endif
