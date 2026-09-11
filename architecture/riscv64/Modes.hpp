#pragma once

#include <architecture/riscv64/Traits.hpp>

namespace QUARK {

struct MachineMode {
  enum {
    PP = 3 << 11,   // Previous Privilege
    PP_M = 3 << 11, // Previous Privilege Machine
    PP_S = 1 << 11, // Previous Privilege Supervisor
    PP_U = 0 << 11, // Previous Privilege User
    PP_SELF = PP_M,

    TW = 1 << 21, // Timeout Wait

    CYCLE = 1 << 0,
    TIME = 1 << 1,
    INSTRET = 1 << 2,

    SI = 1ULL << 3,    // Software Interrupt Enable
    TI = 1ULL << 7,    // Timer Interrupt Enable
    EI = 1ULL << 11,   // External Interrupt Enable
    IRQE = 1ULL << 3,  // Interrupt Enable
    PIRQE = 1ULL << 7, // Previous Interrupt Enable

    /* Registers */
    HARTID = 0xF14,
    STATUS = 0x300,
    MISA = 0x301,
    MEDELEG = 0x302,
    MIDELEG = 0x303,
    IE = 0x304,
    TVEC = 0x305,
    MCOUNTEREN = 0x306,
    PMPADDR0 = 0x3B0,
    PMPCFG0 = 0x3A0,
    PMPADDR1 = 0x3B1,
    PMPCFG1 = 0x3A1,
    SCRATCH = 0x340,
    EPC = 0x341,
    CAUSE = 0x342,
    IP = 0x344,
    TVAL = 0x343,
    VENDORID = 0xF11,
    ARCHID = 0xF12,
    IMPID = 0xF13,
    MCYCLE = 0xB00,
    MINSTRET = 0xB02,
    MHPMCOUNTER3 = 0xB03,
    MCOUNTINHIBIT = 0x320,
    MHPMEVENT3 = 0x323,
  };

  __attribute__((always_inline)) static inline void ret() {
    asm volatile("mret");
  }
};

struct SupervisorMode {
  enum {
    PP = 1ULL << 8,   // Previous Privilege
    PP_S = 1ULL << 8, // Previous Privilege
    PP_U = 0ULL << 8, // User Privilege
    PP_SELF = PP_S,

    IRQE = 1ULL << 1,  // Interrupt Enable
    PIRQE = 1ULL << 5, // Previous Interrupt Enable
    SUM = 1ULL << 18,  // Supervisor User Memory
    SI = 1ULL << 1,    // Software Interrupt Enable
    TI = 1ULL << 5,    // Timer Interrupt Enable
    EI = 1ULL << 9,    // External Interrupt Enable

    /* Registers */
    SATP = 0x180,
    STATUS = 0x100,
    IE = 0x104,
    TVEC = 0x105,
    SCRATCH = 0x140,
    EPC = 0x141,
    CAUSE = 0x142,
    TVAL = 0x143,
    IP = 0x144,
  };

  __attribute__((always_inline)) static inline void ret() {
    asm volatile("sret");
  }
};

using KernelMode =
    Meta::IF<Traits<RISCV>::Supervisor, SupervisorMode, MachineMode>::Result;

} // namespace QUARK
