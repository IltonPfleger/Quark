#ifndef __QUARK_RISCV64_CONTEXT__
#define __QUARK_RISCV64_CONTEXT__

#include <Traits.hpp>
#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/CoreContext.hpp>
#include <architecture/riscv64/FPU.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <memory/Chunk.hpp>

namespace QUARK {

struct UserContext {};
struct KernelContext {};

template <typename T, bool ChangeStack> class ContextTemplate {
  public:
    ContextTemplate(KernelContext, const Chunk &usp, const Chunk &ksp, auto pc, auto a0, auto a1) {
        frame_         = reinterpret_cast<ContextFrame *>(ksp.end()) - 1;
        frame_->status = static_cast<uint64_t>(T::ME2ME);
        frame_->pc     = reinterpret_cast<uint64_t>(pc);
        frame_->a0     = reinterpret_cast<uint64_t>(a0);
        frame_->a1     = reinterpret_cast<uint64_t>(a1);
        frame_->ksp    = usp.end();
    }

    ContextTemplate(UserContext, const Chunk &usp, const Chunk &ksp, auto pc, auto ra, auto a0) {
        frame_         = reinterpret_cast<ContextFrame *>(usp.end()) - 1;
        frame_->status = static_cast<uint64_t>(T::ME2ME | T::PIRQE);
        frame_->pc     = reinterpret_cast<uint64_t>(pc);
        frame_->ra     = reinterpret_cast<uint64_t>(ra);
        frame_->a0     = reinterpret_cast<uint64_t>(a0);
        frame_->ksp    = ksp.end();
    }

    static void load(ContextTemplate &next) {
        asm volatile("mv sp, %0; jr %1" : : "r"(next.frame_), "r"(static_cast<void (*)()>(&ContextTemplate::load)));
    }

    __attribute__((naked)) static void swtch(ContextTemplate &previous, ContextTemplate &next) {
        save();

        asm("sd sp, 0(%0)" ::"r"(&previous.frame_));
        asm("mv sp, %0" ::"r"(next.frame_));

        FPU::swtch<T>(previous.frame_, next.frame_, &previous.fpu_, &next.fpu_);

        load();
    }

    __attribute__((naked)) static void load() {
        if constexpr (ChangeStack) {
            asm("csrr t0, %0" ::"i"(T::SCRATCH));
            asm("ld t1, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, ksp)));
            asm("sd t1, %0(t0)" ::"i"(__builtin_offsetof(CoreContext, ksp)));
        }

        asm("ld t0, %0(sp); csrw %1, t0" ::"i"(__builtin_offsetof(ContextFrame, status)), "i"(T::STATUS));
        asm("ld t0, %0(sp); csrw %1, t0" ::"i"(__builtin_offsetof(ContextFrame, pc)), "i"(T::EPC));

        asm("ld s0,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s0)));
        asm("ld s1,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s1)));
        asm("ld s2,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s2)));
        asm("ld s3,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s3)));
        asm("ld s4,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s4)));
        asm("ld s5,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s5)));
        asm("ld s6,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s6)));
        asm("ld s7,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s7)));
        asm("ld s8,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s8)));
        asm("ld s9,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s9)));
        asm("ld s10, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s10)));
        asm("ld s11, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s11)));

        asm("ld ra,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, ra)));
        asm("ld a0,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a0)));
        asm("ld a1,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a1)));

        asm("addi sp, sp, %0" ::"i"(sizeof(ContextFrame)));

        T::ret();
    }

    __attribute__((always_inline)) static void save() {
        asm("addi sp, sp, %0" ::"i"(-sizeof(ContextFrame)));

        asm("sd s0,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s0)));
        asm("sd s1,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s1)));
        asm("sd s2,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s2)));
        asm("sd s3,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s3)));
        asm("sd s4,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s4)));
        asm("sd s5,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s5)));
        asm("sd s6,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s6)));
        asm("sd s7,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s7)));
        asm("sd s8,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s8)));
        asm("sd s9,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s9)));
        asm("sd s10, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s10)));
        asm("sd s11, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s11)));
        asm("sd ra,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, pc)));

        asm("csrr t0, %0" ::"i"(T::STATUS));
        asm("or   t0, t0, %0" ::"r"(T::ME2ME));
        asm("and  t0, t0, %0" ::"r"(~T::PIRQE));
        asm("sd   t0, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, status)));

        if constexpr (ChangeStack) {
            asm("csrr t0, %0" ::"i"(T::SCRATCH));
            asm("ld t1, %0(t0)" ::"i"(__builtin_offsetof(CoreContext, ksp)));
            asm("sd t1, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, ksp)));
        }
    }

    __attribute__((always_inline)) static inline ContextFrame *push() {
        if constexpr (ChangeStack) {
            asm("csrrw t0, %0, t0" ::"i"(T::SCRATCH));
            asm("sd a0, %0(t0)" ::"i"(__builtin_offsetof(CoreContext, scratch0)));
            asm("sd sp, %0(t0)" ::"i"(__builtin_offsetof(CoreContext, scratch1)));
            asm("ld sp, %0(t0)" ::"i"(__builtin_offsetof(CoreContext, ksp)));
            asm("mv a0, t0");
            asm("csrrw t0, %0, t0" ::"i"(T::SCRATCH));
        }

        asm("addi sp, sp, %0" ::"i"(-sizeof(ContextFrame)));

        asm("sd ra, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, ra)));
        asm("sd gp, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, gp)));
        asm("sd tp, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, tp)));
        asm("sd t0, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, t0)));
        asm("sd t1, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, t1)));
        asm("sd t2, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, t2)));
        asm("sd t3, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, t3)));
        asm("sd t4, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, t4)));
        asm("sd t5, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, t5)));
        asm("sd t6, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, t6)));

        if constexpr (ChangeStack) {
            asm("ld t0, %0(a0)" : : "i"(__builtin_offsetof(CoreContext, scratch0)));
            asm("sd t0, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a0)));
            asm("ld t0, %0(a0)" : : "i"(__builtin_offsetof(CoreContext, scratch1)));
            asm("sd t0, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, sp)));
        } else {
            asm("sd a0, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a0)));
        }

        asm("sd a1, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a1)));
        asm("sd a2, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a2)));
        asm("sd a3, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a3)));
        asm("sd a4, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a4)));
        asm("sd a5, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a5)));
        asm("sd a6, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a6)));
        asm("sd a7, %0(sp)" : : "i"(__builtin_offsetof(ContextFrame, a7)));

        // asm("blt t0, zero, 1f");
        asm("sd s0,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s0)));
        asm("sd s1,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s1)));
        asm("sd s2,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s2)));
        asm("sd s3,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s3)));
        asm("sd s4,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s4)));
        asm("sd s5,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s5)));
        asm("sd s6,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s6)));
        asm("sd s7,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s7)));
        asm("sd s8,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s8)));
        asm("sd s9,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s9)));
        asm("sd s10, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s10)));
        asm("sd s11, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s11)));
        // asm("1:");

        asm("csrr t0, %0; sd t0, %1(sp)" ::"i"(T::CAUSE), "i"(__builtin_offsetof(ContextFrame, cause)));
        asm("csrr t0, %0; sd t0, %1(sp)" ::"i"(T::STATUS), "i"(__builtin_offsetof(ContextFrame, status)));
        asm("csrr t0, %0; sd t0, %1(sp)" ::"i"(T::EPC), "i"(__builtin_offsetof(ContextFrame, pc)));
        asm("csrr t0, %0; sd t0, %1(sp)" ::"i"(T::TVAL), "i"(__builtin_offsetof(ContextFrame, value)));

        register ContextFrame *sp asm("sp");
        return sp;
    }

    __attribute__((always_inline)) static void pop() {
        asm("ld t0, %0(sp); csrw %1, t0" ::"i"(__builtin_offsetof(ContextFrame, status)), "i"(T::STATUS));
        asm("ld t0, %0(sp); csrw %1, t0" ::"i"(__builtin_offsetof(ContextFrame, pc)), "i"(T::EPC));

        // asm("ld t0, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, cause)));
        // asm("blt t0, zero, 1f");
        asm("ld s0,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s0)));
        asm("ld s1,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s1)));
        asm("ld s2,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s2)));
        asm("ld s3,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s3)));
        asm("ld s4,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s4)));
        asm("ld s5,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s5)));
        asm("ld s6,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s6)));
        asm("ld s7,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s7)));
        asm("ld s8,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s8)));
        asm("ld s9,  %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s9)));
        asm("ld s10, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s10)));
        asm("ld s11, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, s11)));
        // asm("1:");

        asm("ld ra, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, ra)));
        asm("ld gp, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, gp)));
        asm("ld tp, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, tp)));
        asm("ld t0, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, t0)));
        asm("ld t1, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, t1)));
        asm("ld t2, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, t2)));
        asm("ld t3, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, t3)));
        asm("ld t4, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, t4)));
        asm("ld t5, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, t5)));
        asm("ld t6, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, t6)));
        asm("ld a0, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a0)));
        asm("ld a1, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a1)));
        asm("ld a2, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a2)));
        asm("ld a3, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a3)));
        asm("ld a4, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a4)));
        asm("ld a5, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a5)));
        asm("ld a6, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a6)));
        asm("ld a7, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, a7)));

        if constexpr (ChangeStack) {
            asm("ld sp, %0(sp)" ::"i"(__builtin_offsetof(ContextFrame, sp)));
        } else {
            asm("addi sp, sp, %0" ::"i"(sizeof(ContextFrame)));
        }

        T::ret();
    }

  protected:
    ContextFrame *frame_;
    FPU fpu_;
};

template <bool ChangeStack = Traits<Thread>::UserStack> using MachineContext = ContextTemplate<MachineMode, ChangeStack>;

template <bool ChangeStack = Traits<Thread>::UserStack> using SupervisorContext = ContextTemplate<SupervisorMode, ChangeStack>;

class VirtualCPU;
class HypervisorContext : public MachineContext<true> {
  public:
    using Father = MachineContext<true>;
    using Father::Father;

    static void swtch(HypervisorContext &, HypervisorContext &);

  private:
    VirtualCPU *cpu_ = nullptr;
};

} // namespace QUARK

#endif
