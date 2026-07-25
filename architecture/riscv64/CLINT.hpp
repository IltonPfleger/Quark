#pragma once

#include <Traits.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/Traits.hpp>
#include <architecture/riscv64/csrs.hpp>

namespace QUARK {

class CLINT {
    enum Registers {
        MTIMECMP = 0x4000,
        MTIME    = 0xBFF8,
    };

  public:
    static uint64_t mtime() { return *mtime_; }

    static void ipi(uint32_t hartid = mhartid()) { msip_[hartid] = hartid != mhartid(); }

    static void write(uint64_t ticks = mtime() + Ticks, uint32_t core = mhartid()) { mtimecmp_[core] = ticks; }

    static void reset(uint64_t delta = Ticks + mtime()) {
        write(delta);
        csrc<MachineMode::IP>(SupervisorMode::TI);
        csrs<MachineMode::IE>(MachineMode::TI);
    }

  public:
    static constexpr unsigned long Address = Traits<CLINT>::Address;
    static constexpr unsigned long Clock   = Traits<CLINT>::Clock;
    static constexpr unsigned long Ticks   = Clock / Traits<QUARK::Timer>::Frequency;

  private:
    static inline volatile uint32_t *msip_     = reinterpret_cast<volatile uint32_t *>(Address);
    static inline volatile uint64_t *mtime_    = reinterpret_cast<volatile uint64_t *>(Address + MTIME);
    static inline volatile uint64_t *mtimecmp_ = reinterpret_cast<volatile uint64_t *>(Address + MTIMECMP);
};

} // namespace QUARK
