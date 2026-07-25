#pragma once

#include <Traits.hpp>
#include <architecture/riscv64/IC.hpp>
#include <architecture/riscv64/IPI.hpp>
#include <architecture/riscv64/PLIC.hpp>
#include <architecture/riscv64/TrapHandler.hpp>
#include <architecture/riscv64/sbi/SBI.hpp>

namespace QUARK {

class HIC {
  public:
    static void init() {
        TrapHandler::init<MachineMode, true>();
        SBI::init();
        PLIC::init();
        TrapHandler::install(11, IC::isr);
        TrapHandler::install(3, IPI::onTrap);
        csrs<MachineMode::IE>(MachineMode::EI);
        csrs<MachineMode::IE>(MachineMode::SI);
    }
};

} // namespace QUARK
