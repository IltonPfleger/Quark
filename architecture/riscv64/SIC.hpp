#pragma once

#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/TrapHandler.hpp>

namespace QUARK {

class SIC {

  public:
    static void init() {
        TrapHandler::init<SupervisorMode, Traits<Thread>::UserStack>();

        if constexpr (Traits<PLIC>::Enable) {
            PLIC::init();
            TrapHandler::install(9, IC::onTrap);
            csrs<SupervisorMode::IE>(SupervisorMode::EI);
        }
    }
};

} // namespace QUARK
