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
      TrapHandler::install(8, syscall, TrapHandler::Type::Exception);
    }

    if constexpr (Traits<PLIC>::Enable) {
      PLIC::init();
      TrapHandler::install(9, IC::isr);
      csrs<SupervisorMode::IE>(SupervisorMode::EI);
    }
  }

private:
  static void syscall(ContextFrame *context) {
    void *result = ABI::Handler::handler(context->a0, &context->a1);
    context->a0 = reinterpret_cast<uintmax_t>(result);
    context->pc += 4;
  }
};

} // namespace QUARK
