#ifndef __QUARK_RISCV64_FPU__
#define __QUARK_RISCV64_FPU__

#include <architecture/riscv64/csrs.hpp>

namespace QUARK {

class FPU {
    struct Registers {
        uint64_t f[32];
        uint64_t fcsr = 0;
    };

  public:
    constexpr FPU() = default;

    static bool enabled(ContextFrame &context) { return context.status & MASK; }

    template <typename T> static void enable(ContextFrame *context) {
        context->status &= ~MASK;
        context->status |= INITIAL;
        csrs<T::STATUS>(INITIAL);
        fcsr(0);
        csrc<T::STATUS>(MASK);
    }

    __attribute__((always_inline)) inline void save() {
        if constexpr (Traits<FPU>::Enable) {
            uint64_t *base = &registers_.f[0];
            asm("fsd f0,   0(%0)\n"
                "fsd f1,   8(%0)\n"
                "fsd f2,  16(%0)\n"
                "fsd f3,  24(%0)\n"
                "fsd f4,  32(%0)\n"
                "fsd f5,  40(%0)\n"
                "fsd f6,  48(%0)\n"
                "fsd f7,  56(%0)\n"
                "fsd f8,  64(%0)\n"
                "fsd f9,  72(%0)\n"
                "fsd f10, 80(%0)\n"
                "fsd f11, 88(%0)\n"
                "fsd f12, 96(%0)\n"
                "fsd f13, 104(%0)\n"
                "fsd f14, 112(%0)\n"
                "fsd f15, 120(%0)\n"
                "fsd f16, 128(%0)\n"
                "fsd f17, 136(%0)\n"
                "fsd f18, 144(%0)\n"
                "fsd f19, 152(%0)\n"
                "fsd f20, 160(%0)\n"
                "fsd f21, 168(%0)\n"
                "fsd f22, 176(%0)\n"
                "fsd f23, 184(%0)\n"
                "fsd f24, 192(%0)\n"
                "fsd f25, 200(%0)\n"
                "fsd f26, 208(%0)\n"
                "fsd f27, 216(%0)\n"
                "fsd f28, 224(%0)\n"
                "fsd f29, 232(%0)\n"
                "fsd f30, 240(%0)\n"
                "fsd f31, 248(%0)\n"
                "csrr t0, fcsr\n"
                "sd   t0, 256(%0)\n"
                :
                : "r"(base)
                : "t0", "memory");
        }
    }

    __attribute__((always_inline)) void load() {
        if constexpr (Traits<FPU>::Enable) {
            uint64_t *base = &registers_.f[0];
            asm("fld f0,   0(%0)\n"
                "fld f1,   8(%0)\n"
                "fld f2,  16(%0)\n"
                "fld f3,  24(%0)\n"
                "fld f4,  32(%0)\n"
                "fld f5,  40(%0)\n"
                "fld f6,  48(%0)\n"
                "fld f7,  56(%0)\n"
                "fld f8,  64(%0)\n"
                "fld f9,  72(%0)\n"
                "fld f10, 80(%0)\n"
                "fld f11, 88(%0)\n"
                "fld f12, 96(%0)\n"
                "fld f13, 104(%0)\n"
                "fld f14, 112(%0)\n"
                "fld f15, 120(%0)\n"
                "fld f16, 128(%0)\n"
                "fld f17, 136(%0)\n"
                "fld f18, 144(%0)\n"
                "fld f19, 152(%0)\n"
                "fld f20, 160(%0)\n"
                "fld f21, 168(%0)\n"
                "fld f22, 176(%0)\n"
                "fld f23, 184(%0)\n"
                "fld f24, 192(%0)\n"
                "fld f25, 200(%0)\n"
                "fld f26, 208(%0)\n"
                "fld f27, 216(%0)\n"
                "fld f28, 224(%0)\n"
                "fld f29, 232(%0)\n"
                "fld f30, 240(%0)\n"
                "fld f31, 248(%0)\n"
                "ld    t0, 256(%0)\n"
                "fscsr t0\n"
                :
                : "r"(base)
                : "t0", "memory");
        }
    }

  public:
    static constexpr uint64_t SHIFT   = 13;
    static constexpr uint64_t MASK    = (3ULL << SHIFT);
    static constexpr uint64_t OFF     = (0ULL << SHIFT);
    static constexpr uint64_t INITIAL = (1ULL << SHIFT);
    static constexpr uint64_t CLEAN   = (2ULL << SHIFT);
    static constexpr uint64_t DIRTY   = (3ULL << SHIFT);

  private:
    Meta::IF<Traits<FPU>::Enable, Registers, Meta::Empty>::Result registers_;
};

} // namespace QUARK

#endif
