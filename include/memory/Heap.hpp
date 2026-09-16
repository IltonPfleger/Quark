#pragma once

#include <Traits.hpp>
#include <memory/Memory.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

struct HeapHeader {
  size_t size;
};

}; // namespace QUARK

extern "C" void *malloc(QUARK::size_t);
extern "C" void free(void *);

inline void *operator new(QUARK::size_t size) { return malloc(size); }
inline void *operator new[](QUARK::size_t size) { return malloc(size); }
inline void operator delete(void *pointer) { free(pointer); }
inline void operator delete[](void *pointer) { free(pointer); }
inline void operator delete[](void *pointer, QUARK::size_t) { free(pointer); }
inline void operator delete(void *pointer, QUARK::size_t) { free(pointer); }
