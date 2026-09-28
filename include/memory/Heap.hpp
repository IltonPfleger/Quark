#ifndef __QUARK_MEMORY_HEAP__
#define __QUARK_MEMORY_HEAP__

#include <Traits.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

enum class Heap { SYSTEM };

}; // namespace QUARK

inline void *operator new(QUARK::size_t size, QUARK::Heap) {
  return QUARK::Memory::alloc(size);
}

inline void *operator new[](QUARK::size_t size, QUARK::Heap) {
  return QUARK::Memory::alloc(size);
}

namespace QUARK {

template <typename T> inline void free(T *pointer, size_t count = 1) {
  static_assert(!Meta::Same<T, void>::Result);
  assert(pointer);
  if constexpr (!Meta::IsTrivial<T>::Result) {
    for (int i = 0; i < count; i++) {
      pointer[i].~T();
    }
  }
  using U = Meta::Remove<T>::Result;
  QUARK::Memory::free(const_cast<U *>(pointer), sizeof(T) * count);
}

} // namespace QUARK

#endif
