#ifndef __QUARK_ABI_HEAP__
#define __QUARK_ABI_HEAP__

#include <Traits.hpp>
#include <abi/ABI.hpp>
#include <architecture/Syscall.hpp>
#include <memory/Memory.hpp>

namespace QUARK::ABI {

struct Allocation {
  size_t size;
};

} // namespace QUARK::ABI

extern "C" inline void *malloc(QUARK::size_t size) {
  using namespace QUARK;

  void *raw;
  size += sizeof(ABI::Allocation);

  if constexpr (Traits<Kernel>::Mode == Traits<Kernel>::KERNEL) {
    raw = Syscall(ABI::ALLOC, size);
  } else {
    raw = Memory::alloc(size);
  }

  auto *allocation = reinterpret_cast<ABI::Allocation *>(raw);
  allocation->size = size;
  return allocation + 1;
}

extern "C" inline void free(void *pointer) {
  using namespace QUARK;

  if (!pointer)
    return;

  auto *allocation = reinterpret_cast<ABI::Allocation *>(pointer) - 1;

  if constexpr (Traits<Kernel>::Mode == Traits<Kernel>::KERNEL) {
    Syscall(ABI::FREE, allocation, allocation->size);
  } else {
    Memory::free(allocation, allocation->size);
  }
}

inline void *operator new(QUARK::size_t size) { return malloc(size); }
inline void *operator new[](QUARK::size_t size) { return malloc(size); }

#endif
