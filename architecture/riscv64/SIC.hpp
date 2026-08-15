#pragma once

#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/TrapHandler.hpp>

namespace QUARK {

class SIC {
  using TrapHandler = QUARK::TrapHandler<SupervisorMode>;

public:
  static void init() {
    TrapHandler::init<Traits<Thread>::UserStack>();

    if constexpr (Traits<Payload>::Unprivileged) {
      TrapHandler::install(8, abi, TrapHandler::Exception);
    }

    if constexpr (Traits<PLIC>::Enable) {
      PLIC::init();
      TrapHandler::install(9, IC::isr);
      csrs<SupervisorMode::IE>(SupervisorMode::EI);
    }
  }

private:
  static void abi(ContextFrame *c) {
    c->a0 = reinterpret_cast<uintptr_t>(
        ABI::Handler::dispatch(static_cast<ABI::Function>(c->a7), &c->a0));
    c->pc += 4;
  }
};

} // namespace QUARK
