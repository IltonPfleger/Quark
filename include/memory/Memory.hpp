#pragma once

#include <Spin.hpp>
#include <Traits.hpp>
#include <memory/allocators/Buddy.hpp>

namespace QUARK {

class Memory {
  private:
    using Allocator = allocators::Buddy<3, Traits<Memory>::Order>;

  public:
    static void init();
    static void *alloc(size_t);
    static void free(void *, size_t);
    static uintptr_t virt2phys(uintptr_t);
    static uintptr_t phys2virt(uintptr_t);

  private:
    static inline Allocator allocator_;
    static inline Spin spin_;
};

} // namespace QUARK
