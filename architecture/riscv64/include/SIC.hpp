#pragma once

#include <architecture/Modes.hpp>
#include <architecture/TrapHandler.hpp>

namespace QUARK {

class SIC {
  using TrapHandler = QUARK::TrapHandler<SupervisorMode>;

public:
  static void init() {
    TrapHandler::init<Traits<Thread>::UserStack>();

    if constexpr (Traits<PLIC>::Enable) {
      PLIC::init();
      TrapHandler::install(9, IC::isr);
      csrs<SupervisorMode::IE>(SupervisorMode::EI);
    }
  }
};

} // namespace QUARK
