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
        TrapHandler<MachineMode>::init<true>();
        SBI::init();
        PLIC::init();
        TrapHandler<MachineMode>::install(11, IC::isr);
        TrapHandler<MachineMode>::install(3, IPI::onTrap);
        csrs<MachineMode::IE>(MachineMode::EI);
        csrs<MachineMode::IE>(MachineMode::SI);
    }
};

} // namespace QUARK
