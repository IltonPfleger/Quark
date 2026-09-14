#ifndef __QUARK_RISCV64_CONTEXT_FRAME__
#define __QUARK_RISCV64_CONTEXT_FRAME__

#include <types.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

struct ContextFrame {
    uint64_t ra, sp, gp, tp;
    uint64_t t0, t1, t2;
    uint64_t s0, s1;
    uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint64_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uint64_t t3, t4, t5, t6;
    uint64_t ksp;
    uint64_t status, cause, value, pc;

    uint64_t &operator[](size_t i) {
        static uint64_t zero = 0;
        if (i == 0) [[unlikely]]
            return zero;
        return (&ra)[i - 1];
    }
};

} // namespace QUARK

#endif
