#ifndef __QUARK_RISCV64_VIRTUAL_CPU__
#define __QUARK_RISCV64_VIRTUAL_CPU__

#include <Traits.hpp>
#include <architecture/CLINT.hpp>
#include <architecture/CPU.hpp>
#include <architecture/IPI.hpp>
#include <architecture/MMU.hpp>
#include <architecture/Modes.hpp>
#include <architecture/PMP.hpp>
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

  enum {
    STI = SupervisorMode::TI,
    SSI = SupervisorMode::SI,
    SEI = SupervisorMode::EI,
    MIDELEG = STI | SSI | SEI,
    PAGE = 1 << 12 | 1 << 13 | 1 << 15,
    ECALL = 1 << 8,
    MISALIGNED = 1 << 4 | 1 << 6,
    BREAKPOINT = 1 << 3,
    MEDELEG = MISALIGNED | BREAKPOINT | ECALL | PAGE,
    RWX = PMP::R | PMP::W | PMP::X,
  };

public:
  enum : uintmax_t {
    FENCEI = 1ULL << 0,
    SFENCE = 1ULL << 1,
    EXTERNAL = 1ULL << 2,
    SOFTWARE = 1ULL << 3,
  };
  VirtualCPU(VirtualMachine *vm) : core_(-1), registers_(), vm_(vm) {}

  void boot(size_t core, void *entry, void *opaque) {
    CPU::IRQ::disable();
    activate();
    restore();

    csrc<MachineMode::STATUS>(SupervisorMode::PIRQE | SupervisorMode::IRQE |
                              MachineMode::PP);

    csrs<MachineMode::STATUS>(MachineMode::PP_S | MachineMode::PIRQE);

    csrs<MachineMode::STATUS>(MachineMode::TW);

    csrw<MachineMode::EPC>(entry);

    CPU::mb();
    CPU::ib();

    dispatch(core, opaque);
  }

  static void kick(size_t core, void *entry, void *opaque) {
    assert(current());
    current()->vm_->boot(core, entry, opaque);
  }

  void set_external_interrupt_pending() {
    int core = core_;
    flags_ |= EXTERNAL;

    if (current() == this) {
      update();
    } else if (core >= 0) {
      IPI::send(core, update);
    }
  }

  void clear_external_interrupt_pending() {
    assert(current() == this);
    csrc<MachineMode::IP>(SEI);
  }

  static bool set_software_interrupt_pending(size_t hartid) {
    if (!current() || hartid >= current()->vm_->cpus())
      return false;

    current()->vm_->cpu(hartid).set_software_interrupt_pending();

    return true;
  }

  static void update(void * = nullptr) {
    VirtualCPU *current = VirtualCPU::current();

    if (!current)
      return;

    if (CLINT::mtime() >= current->registers_.mtimecmp) {
      csrs<MachineMode::IP>(STI);
    } else {
      csrc<MachineMode::IP>(STI);
    }

    const uintmax_t external =
        -((current->flags_.fand(~EXTERNAL) & EXTERNAL) != 0);

    const uintmax_t software =
        -((current->flags_.fand(~SOFTWARE) & SOFTWARE) != 0);

    csrs<MachineMode::IP>((SEI & external) | (SSI & software));
  }

  static void mtimecmp(uintmax_t mtimecmp) {
    assert(current());
    current()->registers_.mtimecmp = mtimecmp;
    update();
  }

  static bool read(uintptr_t address, uint32_t *destination) {
    assert(current());
    return current()->vm_->read(address, destination, sizeof(uint32_t));
  }

  static bool write(uintptr_t address, uint32_t source) {
    assert(current());
    return current()->vm_->write(address, &source, sizeof(uint32_t));
  }

  static bool sb(uintmax_t address, uint8_t source) {
    assert(current());
    if (!current()->vm_->memory().contains(Chunk(address, 1))) [[unlikely]]
      return false;
    *reinterpret_cast<uint8_t *>(address) = source;
    return true;
  }

  static bool lb(uintmax_t address, uint8_t *source) {
    assert(current());
    if (!current()->vm_->memory().contains(Chunk(address, 1))) [[unlikely]]
      return false;
    *source = *reinterpret_cast<uint8_t *>(address);
    return true;
  }

  static VirtualCPU *swtch(VirtualCPU *next) {
    VirtualCPU *previous = current();

    if (previous) {
      previous->save();
      previous->core_ = -1;
    }

    if (next) {
      next->restore();
      next->activate();
    } else {
      csrc<MachineMode::IE>(MIDELEG);
      csrc<MachineMode::IP>(MIDELEG);
      csrw<MachineMode::MIDELEG>(0);
      csrw<MachineMode::MEDELEG>(0);
      current(nullptr);
    }

    return previous;
  }

  static void fence(size_t hartid, uintmax_t flags) {
    assert(current());

    VirtualCPU &destination = current()->vm_->cpu(hartid);

    destination.flags_ |= flags;

    int core = destination.core_;

    if (&destination == current()) {
      destination.fence();
    } else if (core >= 0) {
      IPI::send(core, fence);
    }
  }

private:
  static void dispatch(size_t core, void *opaque) {
    register size_t a0 asm("a0") = core;
    register void *a1 asm("a1") = opaque;
    asm volatile("mret" : : "r"(a0), "r"(a1));
  }

  void set_software_interrupt_pending() {
    int core = core_;
    flags_ |= SOFTWARE;

    if (current() == this) {
      update();
    } else if (core >= 0) {
      IPI::send(core, update);
    }
  }

  void activate() {
    PMP::NAPOT<1>(vm_->memory().start(), vm_->memory().length(), RWX);

    csrw<MachineMode::MIDELEG>(MIDELEG);
    csrw<MachineMode::MEDELEG>(MEDELEG);

    core_ = mhartid();
    current(this);
    update();
    fence();
  }

  void save() {
    registers_.sscratch = csrr<SupervisorMode::SCRATCH>();
    registers_.satp = csrr<SupervisorMode::SATP>();
    registers_.stvec = csrr<SupervisorMode::TVEC>();
    registers_.scause = csrr<SupervisorMode::CAUSE>();
    registers_.stval = csrr<SupervisorMode::TVAL>();
    registers_.sepc = csrr<SupervisorMode::EPC>();
    registers_.sie = csrr<MachineMode::IE>() & MIDELEG;
    registers_.sip = csrr<MachineMode::IP>() & MIDELEG;
  }

  void restore() {
    csrw<SupervisorMode::SCRATCH>(registers_.sscratch);
    csrw<SupervisorMode::SATP>(registers_.satp);
    csrw<SupervisorMode::TVEC>(registers_.stvec);
    csrw<SupervisorMode::CAUSE>(registers_.scause);
    csrw<SupervisorMode::TVAL>(registers_.stval);
    csrw<SupervisorMode::EPC>(registers_.sepc);
    csrc<MachineMode::IE>(MIDELEG);
    csrs<MachineMode::IE>(registers_.sie);
    csrc<MachineMode::IP>(MIDELEG);
    csrs<MachineMode::IP>(registers_.sip);
    MMU::TLB::flush();
  }

  static void fence(void *) {
    if (!current())
      return;
    current()->fence();
  }

  void fence() {
    if (flags_ & FENCEI) {
      flags_ &= ~FENCEI;
      CPU::ib();
    }
    if (flags_ & SFENCE) {
      flags_ &= ~SFENCE;
      MMU::TLB::flush();
    }
  }

  static void current(VirtualCPU *current) { current_[CPU::id()] = current; }
  static VirtualCPU *current() { return current_[CPU::id()]; }

private:
  static constinit inline VirtualCPU *current_[Traits<CPU>::Active] = {};

private:
  int core_;
  Atomic<uintmax_t> flags_;
  Registers registers_;
  VirtualMachine *vm_;
};

} // namespace QUARK

#endif
