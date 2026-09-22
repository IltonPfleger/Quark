#pragma once

#include <Traits.hpp>
#include <architecture/IC.hpp>
#include <architecture/IPI.hpp>
#include <architecture/PLIC.hpp>
#include <architecture/TrapHandler.hpp>
#include <architecture/sbi/SBI.hpp>

namespace QUARK {

class HIC {
public:
  static void init() {
    TrapHandler<MachineMode>::init<true>();
    SBI::init();
    PLIC::init();
    TrapHandler<MachineMode>::install(11, IC::isr);
    TrapHandler<MachineMode>::install(3, IPI::isr);
    csrs<MachineMode::IE>(MachineMode::EI);
    csrs<MachineMode::IE>(MachineMode::SI);
  }
};

} // namespace QUARK
