#pragma once

#include <abi/ABI.hpp>
#include <architecture/Syscall.hpp>
#include <types.hpp>

/***** New *****/
inline void *operator new(QUARK::size_t size) {
  return QUARK::Syscall(QUARK::ABI::Function::ABI_HEAP_NEW, size);
}
inline void *operator new[](QUARK::size_t size) { return ::operator new(size); }

/***** Delete *****/
inline void operator delete(void *pointer, QUARK::size_t size) {
  QUARK::Syscall(QUARK::ABI::Function::ABI_HEAP_DELETE, pointer);
}

inline void operator delete[](void *pointer, QUARK::size_t size) {
  ::operator delete(pointer, size);
}
