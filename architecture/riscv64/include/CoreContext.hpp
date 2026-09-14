#ifndef __QUARK_RISCV64_CORE_CONTEXT__
#define __QUARK_RISCV64_CORE_CONTEXT__

#include <types.hpp>

namespace QUARK {

struct CoreContext {
    uintptr_t scratch0;
    uintptr_t scratch1;
    uintptr_t ksp;
    uintptr_t core;
};

} // namespace QUARK

#endif
