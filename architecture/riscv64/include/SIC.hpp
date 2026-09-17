#pragma once

#include <abi/Handler.hpp>
#include <architecture/Modes.hpp>
#include <architecture/TrapHandler.hpp>

namespace QUARK {

class SIC {
  using TrapHandler = QUARK::TrapHandler<SupervisorMode>;

public:
  static void init() {
    TrapHandler::init<Traits<Thread>::UserStack>();

    if constexpr (Traits<Kernel>::Mode == Traits<Kernel>::KERNEL) {
      TrapHandler::install(9, syscall, TrapHandler::Type::Exception);
    }

    if constexpr (Traits<PLIC>::Enable) {
      PLIC::init();
      TrapHandler::install(9, IC::isr);
      csrs<SupervisorMode::IE>(SupervisorMode::EI);
    }
  }

private:
  static void syscall(ContextFrame *context) {
    ABI::Handler::handler(context->a7, &context->a0);
  }
};

} // namespace QUARK
