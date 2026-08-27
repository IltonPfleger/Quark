#pragma once

#include <Traits.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

enum class Heap { APPLICATION, SYSTEM };

/**** New ****/
inline void *operator new(QUARK::size_t size, Heap) {
  return QUARK::Memory::alloc(size);
}

inline void *operator new(QUARK::size_t size) {
  return ::operator new(size, Heap::APPLICATION);
}

inline void *operator new[](QUARK::size_t size, Heap selector) {
  void *raw = QUARK::Memory::alloc(size + sizeof(QUARK::size_t));
  auto sized = reinterpret_cast<QUARK::size_t *>(raw);
  *sized = size;
  return sized + 1;
}

inline void *operator new[](QUARK::size_t size) {
  return ::operator new[](size, Heap::APPLICATION);
}

/**** Delete ****/
inline void operator delete(void *pointer, QUARK::size_t size, Heap) {
  QUARK::Memory::free(pointer, size);
}

inline void operator delete(void *pointer, QUARK::size_t size) {
  ::operator delete(pointer, size, Heap::APPLICATION);
}

inline void operator delete[](void *pointer, Heap selector) {
  if (!pointer)
    return;
  auto sized = reinterpret_cast<QUARK::size_t *>(pointer);
  auto size = *(sized - 1) + sizeof(QUARK::size_t);
  ::operator delete(sized - 1, size, selector);
}

inline void operator delete[](void *pointer) {
  ::operator delete[](pointer, Heap::APPLICATION);
}

inline void operator delete[](void *pointer, QUARK::size_t) {
  ::operator delete[](pointer, Heap::APPLICATION);
}
