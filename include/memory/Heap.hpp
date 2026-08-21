#pragma once

#include <Traits.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

enum class Heap { APPLICATION, SYSTEM };

inline void *operator new(QUARK::size_t size, Heap) {
  return QUARK::Memory::alloc(size);
}

inline void *operator new(QUARK::size_t size) {
  return ::operator new(size, Heap::APPLICATION);
}

inline void *operator new[](QUARK::size_t size) {
  return ::operator new(size, Heap::APPLICATION);
}

inline void *operator new[](QUARK::size_t size, Heap selector) {
  return ::operator new(size, selector);
}

/**** Delete ****/
inline void operator delete(void *pointer, QUARK::size_t size, Heap) {
  QUARK::Memory::free(pointer, size);
}

inline void operator delete(void *pointer, QUARK::size_t size) {
  ::operator delete(pointer, size, Heap::APPLICATION);
}

inline void operator delete[](void *pointer, QUARK::size_t size) {
  ::operator delete(pointer, size);
}

inline void operator delete[](void *pointer, QUARK::size_t size,
                              Heap selector) {
  ::operator delete(pointer, size, selector);
}

//
// struct HeapHeader {
//   QUARK::size_t size;
// };
//
////-----------------------------------------------------------------------------
//// New
////-----------------------------------------------------------------------------
//
// inline void *operator new(QUARK::size_t size, Heap selector) {
//  const QUARK::size_t total = sizeof(HeapHeader) + size;
//
//  auto *raw = static_cast<unsigned char *>(QUARK::Memory::alloc(total));
//
//  auto *header = reinterpret_cast<HeapHeader *>(raw);
//  header->size = total;
//
//  return header + 1;
//}
//
// inline void *operator new(QUARK::size_t size) {
//  return ::operator new(size, Heap::APPLICATION);
//}
//
// inline void *operator new[](QUARK::size_t size, Heap selector) {
//  return ::operator new(size, selector);
//}
//
// inline void *operator new[](QUARK::size_t size) {
//  return ::operator new(size, Heap::APPLICATION);
//}
//
////-----------------------------------------------------------------------------
//// Delete
////-----------------------------------------------------------------------------
//
// inline void operator delete(void *pointer)  {
//  if (!pointer) {
//    return;
//  }
//
//  auto *header = reinterpret_cast<HeapHeader *>(pointer) - 1;
//  QUARK::Memory::free(header, header->size);
//}
//
// inline void operator delete(void *pointer, QUARK::size_t)  {
//  ::operator delete(pointer);
//}
//
// inline void operator delete(void *pointer, Heap)  {
//  ::operator delete(pointer);
//}
//
// inline void operator delete(void *pointer, QUARK::size_t, Heap)  {
//  ::operator delete(pointer);
//}
//
// inline void operator delete[](void *pointer)  {
//  ::operator delete(pointer);
//}
//
// inline void operator delete[](void *pointer, QUARK::size_t)  {
//  ::operator delete(pointer);
//}
//
// inline void operator delete[](void *pointer, Heap)  {
//  ::operator delete(pointer);
//}
//
// inline void operator delete[](void *pointer, QUARK::size_t, Heap)  {
//  ::operator delete(pointer);
//}
