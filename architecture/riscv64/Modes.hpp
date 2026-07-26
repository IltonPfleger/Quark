#pragma once

#include <architecture/riscv64/Traits.hpp>

namespace QUARK {

struct MachineMode {
    enum {
        PP      = 3 << 11, // Previous Privilege
        PP_M    = 3 << 11, // Previous Privilege Machine
        PP_S    = 1 << 11, // Previous Privilege Supervisor
        PP_U    = 0 << 11, // Previous Privilege User
        PP_SELF = PP_M,

        TW = 1 << 21, // Timeout Wait

        CYCLE   = 1 << 0,
        TIME    = 1 << 1,
        INSTRET = 1 << 2,

        SI    = 1ULL << 3,  // Software Interrupt Enable
        TI    = 1ULL << 7,  // Timer Interrupt Enable
        EI    = 1ULL << 11, // External Interrupt Enable
        IRQE  = 1ULL << 3,  // Interrupt Enable
        PIRQE = 1ULL << 7,  // Previous Interrupt Enable
    };

    static constexpr int HARTID     = 0xF14;
    static constexpr int STATUS     = 0x300;
    static constexpr int MISA       = 0x301;
    static constexpr int MEDELEG    = 0x302;
    static constexpr int MIDELEG    = 0x303;
    static constexpr int IE         = 0x304;
    static constexpr int TVEC       = 0x305;
    static constexpr int MCOUNTEREN = 0x306;
    static constexpr int PMPADDR0   = 0x3B0;
    static constexpr int PMPCFG0    = 0x3A0;
    static constexpr int PMPADDR1   = 0x3B1;
    static constexpr int PMPCFG1    = 0x3A1;
    static constexpr int SCRATCH    = 0x340;
    static constexpr int EPC        = 0x341;
    static constexpr int CAUSE      = 0x342;
    static constexpr int IP         = 0x344;
    static constexpr int TVAL       = 0x343;
    static constexpr int VENDORID   = 0xF11;
    static constexpr int ARCHID     = 0xF12;
    static constexpr int IMPID      = 0xF13;

    static constexpr char PREFIX = 'm';

    __attribute__((always_inline)) static inline void ret() { asm volatile("mret"); }
};

struct SupervisorMode {
    enum {
        PP      = 1ULL << 8, // Previous Privilege
        PP_S    = 1ULL << 8, // Previous Privilege
        PP_U    = 0ULL << 8, // User Privilege
        PP_SELF = PP_U,

        IRQE  = 1ULL << 1,  // Interrupt Enable
        PIRQE = 1ULL << 5,  // Previous Interrupt Enable
        SUM   = 1ULL << 18, // Supervisor User Memory
        SI    = 1ULL << 1,  // Software Interrupt Enable
        TI    = 1ULL << 5,  // Timer Interrupt Enable
        EI    = 1ULL << 9,  // External Interrupt Enable
    };

    static constexpr const int SATP    = 0x180;
    static constexpr const int STATUS  = 0x100;
    static constexpr const int IE      = 0x104;
    static constexpr const int TVEC    = 0x105;
    static constexpr const int SCRATCH = 0x140;
    static constexpr const int EPC     = 0x141;
    static constexpr const int CAUSE   = 0x142;
    static constexpr const int TVAL    = 0x143;
    static constexpr const int IP      = 0x144;
    static constexpr char PREFIX       = 's';

    __attribute__((always_inline)) static inline void ret() { asm volatile("sret"); }
};

using KernelMode = Meta::IF<Traits<RISCV>::Supervisor, SupervisorMode, MachineMode>::Result;

} // namespace QUARK
