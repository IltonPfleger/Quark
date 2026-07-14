#ifndef __QUARK_RISCV64_VCPU__
#define __QUARK_RISCV64_VCPU__

#include <Traits.hpp>
#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/CPU.hpp>
#include <architecture/riscv64/MMU.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/PMP.hpp>
#include <hypervisor/VirtualMachine.hpp>
#include <utility/Atomic.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

class VirtualCPU {
    struct Registers {
        uint64_t mtimecmp = ~0ULL;
        uint64_t sscratch = 0;
        uint64_t satp     = 0;
        uint64_t stvec    = 0;
        uint64_t scause   = 0;
        uint64_t stval    = 0;
        uint64_t sepc     = 0;
        uint64_t sie      = 0;
        uint64_t sip      = 0;
    };

  public:
    VirtualCPU(VirtualMachine *vm)
        : core_(-1),
          registers_(),
          vm_(vm) {}

    void boot(size_t core, auto... args) {
        CPU::IRQ::disable();
        activate();
        restore();
        csrs<MachineMode::STATUS>(MachineMode::TW | MachineMode::PP_S | MachineMode::PIRQE);
        csrc<MachineMode::STATUS>(SupervisorMode::PIRQE | SupervisorMode::IRQE);
        csrw<MachineMode::EPC>(vm_->memory().start());
        dispatch(core, args...);
    }

    void activate() {
        PMP::NAPOT<1>(vm_->memory().start(), vm_->memory().size(), PMP::R | PMP::W | PMP::X);

        csrw<MachineMode::MIDELEG>(MIDELEG);
        csrw<MachineMode::MEDELEG>(MEDELEG);

        core_ = mhartid();
        current(this);

        if (CLINT::mtime() >= registers_.mtimecmp) registers_.sip |= SupervisorMode::TI;
        if (external_) registers_.sip |= SupervisorMode::EI;
    }

    void setInterruptPending() {
        int core  = core_;
        external_ = true;
        if (current() == this)
            setExternalInterruptPending();
        else if (core >= 0)
            CLINT::ipi(core);
    }

    void clearInterruptPending() {
        assert(current() == this);
        external_ = false;
        clearExternalInterruptPending();
    }

    static void onInterProcessorInterrupt() {
        if (current() && current()->external_) csrs<MachineMode::IP>(SupervisorMode::EI);
    }

    static void onTick() {
        if (!current()) return;
        if (CLINT::mtime() >= current()->registers_.mtimecmp) {
            setTimerInterruptPending();
        }
    }

    static uintmax_t mtimecmp() {
        assert(current());
        return current()->registers_.mtimecmp;
    }

    static void mtimecmp(uintmax_t mtimecmp) {
        assert(current());
        current()->registers_.mtimecmp = mtimecmp;
        if (mtimecmp <= CLINT::mtime()) {
            setTimerInterruptPending();
        } else {
            clearTimerInterruptPending();
        }
    }

    static bool read(uintptr_t address, uint32_t *destination) {
        if (!current()) return false;
        return current()->vm_->read(address, destination);
    }

    static bool write(uintptr_t address, uint32_t source) {
        if (!current()) return false;
        return current()->vm_->write(address, source);
    }

    static VirtualCPU *current() { return current_[CPU::id()]; }

    static void swtch(VirtualCPU *previous, VirtualCPU *next) {
        if (previous) {
            previous->save();
            previous->core_ = -1;
        }
        if (next) {
            next->activate();
            next->restore();
        } else {
            csrc<MachineMode::IE>(0x222);
            csrc<MachineMode::IP>(0x222);
            csrw<MachineMode::MIDELEG>(0);
            csrw<MachineMode::MEDELEG>(0);
            current(nullptr);
        }
    }

  private:
    __attribute__((naked)) static void dispatch(auto... args) {
        ((void)args, ...);
        asm("mret");
    }

    void save() {
        registers_.sscratch = csrr<SupervisorMode::SCRATCH>();
        registers_.satp     = csrr<SupervisorMode::SATP>();
        registers_.stvec    = csrr<SupervisorMode::TVEC>();
        registers_.scause   = csrr<SupervisorMode::CAUSE>();
        registers_.stval    = csrr<SupervisorMode::TVAL>();
        registers_.sepc     = csrr<SupervisorMode::EPC>();
        registers_.sie      = csrr<MachineMode::IE>();
        registers_.sip      = csrr<MachineMode::IP>();
    }

    void restore() {
        csrw<SupervisorMode::SCRATCH>(registers_.sscratch);
        csrw<SupervisorMode::SATP>(registers_.satp);
        csrw<SupervisorMode::TVEC>(registers_.stvec);
        csrw<SupervisorMode::CAUSE>(registers_.scause);
        csrw<SupervisorMode::TVAL>(registers_.stval);
        csrw<SupervisorMode::EPC>(registers_.sepc);
        csrc<MachineMode::IE>(0x222);
        csrs<MachineMode::IE>(registers_.sie & 0x222);
        csrc<MachineMode::IP>(0x222);
        csrs<MachineMode::IP>(registers_.sip & 0x222);
        MMU::TLB::flush();
    }

    static void setTimerInterruptPending() { csrs<MachineMode::IP>(SupervisorMode::TI); }

    static void clearTimerInterruptPending() { csrc<MachineMode::IP>(SupervisorMode::TI); }

    static void setExternalInterruptPending() { csrs<MachineMode::IP>(SupervisorMode::EI); }

    static void clearExternalInterruptPending() { csrc<MachineMode::IP>(SupervisorMode::EI); }

    static void current(VirtualCPU *current) { current_[CPU::id()] = current; }

  private:
    static constinit inline VirtualCPU *current_[Traits<CPU>::Active] = {};

    static constexpr uintmax_t MIDELEG = SupervisorMode::SI | SupervisorMode::TI | SupervisorMode::EI;
    static constexpr uintmax_t MEDELEG = 1 << 3     // Breakpoint
                                         | 1 << 4   // Load Address Misaligned
                                         | 1 << 6   // Store Address Misaligned
                                         | 1 << 8   // Environment Call From U-Mode
                                         | 1 << 12  // Instruction Page Fault
                                         | 1 << 13  // Load Page Fault
                                         | 1 << 15; // Store Page Fault

  private:
    int core_;
    bool external_;
    Registers registers_;
    VirtualMachine *vm_;
};

} // namespace QUARK

#endif
