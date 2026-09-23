#ifndef __QUARK_MEMORY_HEAP__
#define __QUARK_MEMORY_HEAP__

#include <Traits.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

struct HeapHeader {
  size_t size;
};

enum class Heap { SYSTEM };

}; // namespace QUARK

inline void *operator new(QUARK::size_t size, QUARK::Heap) {
  return QUARK::Memory::alloc(size);
}

namespace QUARK {

template <typename T> inline void free(T *pointer) {
  static_assert(!Meta::Same<T, void>::Result);
  assert(pointer);
  pointer->~T();
  QUARK::Memory::free(pointer, sizeof(T));
}

} // namespace QUARK

#endif
