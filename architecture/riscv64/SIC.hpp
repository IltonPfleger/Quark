#pragma once

#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/TrapHandler.hpp>

namespace QUARK {

class SIC {
    using TrapHandler = QUARK::TrapHandler<SupervisorMode>;

  public:
    static void init() {
        TrapHandler::init<Traits<Thread>::UserStack>();
        // for (size_t i = 0; i < TrapHandler::NumberOfExceptions; i++) {
        //     TrapHandler::install(i, ExceptionHandler::esr, TrapHandler::Exception);
        // }

        if constexpr (Traits<PLIC>::Enable) {
            PLIC::init();
            TrapHandler::install(9, IC::isr);
            csrs<SupervisorMode::IE>(SupervisorMode::EI);
        }
    }
};

} // namespace QUARK
