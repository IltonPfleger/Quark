#pragma once

#include <Traits.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

enum class Heap { APPLICATION, SYSTEM };

struct HeapHeader {
  unsigned long size;
};

//-----------------------------------------------------------------------------
// New
//-----------------------------------------------------------------------------

inline void *operator new(unsigned long size, Heap selector) {
  const unsigned long total = sizeof(HeapHeader) + size;

  auto *raw = static_cast<unsigned char *>(QUARK::Memory::alloc(total));

  auto *header = reinterpret_cast<HeapHeader *>(raw);
  header->size = total;

  return header + 1;
}

inline void *operator new(unsigned long size) {
  return ::operator new(size, Heap::APPLICATION);
}

inline void *operator new[](unsigned long size, Heap selector) {
  return ::operator new(size, selector);
}

inline void *operator new[](unsigned long size) {
  return ::operator new(size, Heap::APPLICATION);
}

//-----------------------------------------------------------------------------
// Delete
//-----------------------------------------------------------------------------

inline void operator delete(void *pointer) noexcept {
  if (!pointer) {
    return;
  }

  auto *header = reinterpret_cast<HeapHeader *>(pointer) - 1;
  QUARK::Memory::free(header, header->size);
}

inline void operator delete(void *pointer, unsigned long) noexcept {
  ::operator delete(pointer);
}

inline void operator delete(void *pointer, Heap) noexcept {
  ::operator delete(pointer);
}

inline void operator delete(void *pointer, unsigned long, Heap) noexcept {
  ::operator delete(pointer);
}

inline void operator delete[](void *pointer) noexcept {
  ::operator delete(pointer);
}

inline void operator delete[](void *pointer, unsigned long) noexcept {
  ::operator delete(pointer);
}

inline void operator delete[](void *pointer, Heap) noexcept {
  ::operator delete(pointer);
}

inline void operator delete[](void *pointer, unsigned long, Heap) noexcept {
  ::operator delete(pointer);
}
