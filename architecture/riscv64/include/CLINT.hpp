#pragma once

#include <Traits.hpp>
#include <architecture/Modes.hpp>
#include <architecture/Traits.hpp>
#include <architecture/csrs.hpp>

namespace QUARK {

class CLINT {
  enum Registers {
    MTIMECMP = 0x4000,
    MTIME = 0xBFF8,
  };

public:
  static uint64_t mtime() { return *mtime_; }

  static void ipi(uint32_t hartid, uint32_t value) { msip_[hartid] = value; }
  [[nodiscard]] static uint32_t ipi(uint32_t hartid) { return msip_[hartid]; }

  static void write(uint64_t ticks = mtime() + kTicks,
                    uint32_t core = mhartid()) {
    mtimecmp_[core] = ticks;
  }

  static void reset(uint64_t delta = kTicks + mtime()) {
    write(delta);
    csrc<MachineMode::IP>(SupervisorMode::TI);
    csrs<MachineMode::IE>(MachineMode::TI);
  }

public:
  static constexpr unsigned long kAddress = Traits<CLINT>::Address;
  static constexpr unsigned long kClock = Traits<CLINT>::Clock;
  static constexpr unsigned long kTicks =
      kClock / Traits<QUARK::Timer>::Frequency;

private:
  static inline volatile uint32_t *msip_ =
      reinterpret_cast<volatile uint32_t *>(kAddress);
  static inline volatile uint64_t *mtime_ =
      reinterpret_cast<volatile uint64_t *>(kAddress + MTIME);
  static inline volatile uint64_t *mtimecmp_ =
      reinterpret_cast<volatile uint64_t *>(kAddress + MTIMECMP);
};

} // namespace QUARK
