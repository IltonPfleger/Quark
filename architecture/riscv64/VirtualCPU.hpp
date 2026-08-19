#ifndef __QUARK_RISCV64_VIRTUAL_CPU__
#define __QUARK_RISCV64_VIRTUAL_CPU__

#include <Traits.hpp>
#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/CPU.hpp>
#include <architecture/riscv64/IPI.hpp>
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
    uint64_t satp = 0;
    uint64_t stvec = 0;
    uint64_t scause = 0;
    uint64_t stval = 0;
    uint64_t sepc = 0;
    uint64_t sie = 0;
    uint64_t sip = 0;
  };

public:
  VirtualCPU(VirtualMachine *vm) : core_(-1), registers_(), vm_(vm) {}

  void boot(size_t core, void *entry, void *opaque) {
    CPU::IRQ::disable();
    activate();
    restore();

    csrc<MachineMode::STATUS>(SupervisorMode::PIRQE | SupervisorMode::IRQE |
                              MachineMode::PP);

    csrs<MachineMode::STATUS>(MachineMode::PP_S | MachineMode::PIRQE);

    // csrs<MachineMode::STATUS>(MachineMode::TW | MachineMode::PP_S |
    //                           MachineMode::PIRQE);

    csrw<MachineMode::EPC>(entry);

    CPU::mb();
    CPU::ib();

    dispatch(core, opaque);
  }

  static void kick(size_t core, void *entry, void *opaque) {
    assert(current());
    current()->vm_->boot(core, entry, opaque);
  }

  void activate() {
    PMP::NAPOT<1>(vm_->memory().start(), vm_->memory().length(),
                  PMP::R | PMP::W | PMP::X);

    csrw<MachineMode::MIDELEG>(MIDELEG);
    csrw<MachineMode::MEDELEG>(MEDELEG);

    core_ = mhartid();
    current(this);

    if (CLINT::mtime() >= registers_.mtimecmp)
      registers_.sip |= SupervisorMode::TI;
  }

  void setInterruptPending() {
    registers_.sip |= SupervisorMode::EI;
    if (current() == this) {
      setExternalInterruptPending();
    } else if (core_ >= 0) {
      IPI::send(core_, onInterProcessorInterrupt);
    }
  }

  void clearInterruptPending() {
    assert(current() == this);
    registers_.sip &= ~SupervisorMode::EI;
    clearExternalInterruptPending();
  }

  void interProcessorInterrupt() {
    registers_.sip |= SupervisorMode::SI;
    if (current() == this) {
      setSoftwareInterruptPending();
    } else if (core_ >= 0) {
      IPI::send(core_, onInterProcessorInterrupt);
    }
  }

  static void interProcessorInterrupt(size_t id) {
    assert(current());
    current()->vm_->cpu(id).interProcessorInterrupt();
  }

  static void onInterProcessorInterrupt(void *) {
    if (!current())
      return;
    csrs<MachineMode::IP>(current()->registers_.sip & 0x222);
  }

  static void onTick() {
    if (!current())
      return;
    // if (CPU::id() == 1)
    //   Console::println(CLINT::mtime(), " ", current()->registers_.mtimecmp);
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

    // if (CPU::id() == 1)
    //   Console::println("MTIMECMP: ", CLINT::mtime(), " ",
    //                    current()->registers_.mtimecmp);

    if (mtimecmp <= CLINT::mtime()) {
      setTimerInterruptPending();
    } else {
      clearTimerInterruptPending();
    }
  }

  static bool read(uintptr_t address, uint32_t *destination) {
    assert(current());
    return current()->vm_->read(address, destination);
  }

  static bool write(uintptr_t address, uint32_t source) {
    assert(current());
    return current()->vm_->write(address, source);
  }

  static bool sb(uintmax_t address, uint8_t source) {
    assert(current());
    if (!current()->vm_->memory().contains(Chunk(address, 1)))
      return false;
    *reinterpret_cast<uint8_t *>(address) = source;
    return true;
  }

  static bool lb(uintmax_t address, uint8_t *source) {
    assert(current());
    if (!current()->vm_->memory().contains(Chunk(address, 1)))
      return false;
    *source = *reinterpret_cast<uint8_t *>(address);
    return true;
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
  static void dispatch(size_t core, void *opaque) {
    asm("mv a0, %0; mv a1, %1; mret" ::"r"(core), "r"(opaque));
  }

  void save() {
    registers_.sscratch = csrr<SupervisorMode::SCRATCH>();
    registers_.satp = csrr<SupervisorMode::SATP>();
    registers_.stvec = csrr<SupervisorMode::TVEC>();
    registers_.scause = csrr<SupervisorMode::CAUSE>();
    registers_.stval = csrr<SupervisorMode::TVAL>();
    registers_.sepc = csrr<SupervisorMode::EPC>();
    registers_.sie = csrr<MachineMode::IE>();
    registers_.sip = csrr<MachineMode::IP>();
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

  static void setSoftwareInterruptPending() {
    csrs<MachineMode::IP>(SupervisorMode::SI);
  }

  static void clearSoftwareInterruptPending() {
    csrc<MachineMode::IP>(SupervisorMode::SI);
  }

  static void setTimerInterruptPending() {
    csrs<MachineMode::IP>(SupervisorMode::TI);
  }

  static void clearTimerInterruptPending() {
    csrc<MachineMode::IP>(SupervisorMode::TI);
  }

  static void setExternalInterruptPending() {
    csrs<MachineMode::IP>(SupervisorMode::EI);
  }

  static void clearExternalInterruptPending() {
    csrc<MachineMode::IP>(SupervisorMode::EI);
  }

  static void current(VirtualCPU *current) { current_[CPU::id()] = current; }

private:
  static constexpr uintmax_t MIDELEG =
      SupervisorMode::SI | SupervisorMode::TI | SupervisorMode::EI;

  static constexpr uintmax_t PAGE = 1 << 12 | 1 << 13 | 1 << 15;
  static constexpr uintmax_t ECALL = 1 << 8;
  static constexpr uintmax_t MISALIGNED = 1 << 4 | 1 << 6;
  static constexpr uintmax_t BREAKPOINT = 1 << 3;
  static constexpr uintmax_t MEDELEG = MISALIGNED | BREAKPOINT | ECALL | PAGE;

private:
  static constinit inline VirtualCPU *current_[Traits<CPU>::Active] = {};

private:
  int core_;
  Registers registers_;
  VirtualMachine *vm_;
};

} // namespace QUARK

#endif
