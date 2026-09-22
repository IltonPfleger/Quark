#pragma once

#include <Traits.hpp>
#include <architecture/CLINT.hpp>
#include <architecture/Decoder.hpp>
#include <architecture/FPU.hpp>
#include <architecture/IC.hpp>
#include <architecture/PLIC.hpp>
#include <architecture/TrapHandler.hpp>

namespace QUARK {

class MIC {
  static constexpr bool IsMachineMode = !Traits<RISCV>::Supervisor;
  static constexpr bool IsTimerEnable = Traits<QUARK::Timer>::Enable;
  static constexpr bool ChangeStack =
      (IsMachineMode && Traits<Thread>::UserStack) || !IsMachineMode;
  using MachineContext = QUARK::MachineContext<ChangeStack>;
  using TrapHandler = QUARK::TrapHandler<MachineMode>;

public:
  static void init() {
    TrapHandler::init<ChangeStack>();

    if constexpr (Traits<RISCV>::Supervisor) {
      CoreContextHandler<MachineMode>::stack(
          __amm.end() - Traits<Memory>::PageSize * CPU::id<true>());

      if constexpr (Traits<QUARK::Timer>::Enable) {
        csrs<MachineMode::IP>(SupervisorMode::TI);
        TrapHandler::install(7, forward);
        TrapHandler::install(9, sbi, TrapHandler::Exception);
      }

      return;
    }

    if constexpr (Traits<PLIC>::Enable) {
      PLIC::init();
      TrapHandler::install(11, IC::isr);
      csrs<MachineMode::IE>(MachineMode::EI);
    }

    if constexpr (Traits<FPU>::Enable) {
      TrapHandler::install(2, fpu, TrapHandler::Exception);
    }
  }

private:
  static void forward(ContextFrame *) {
    csrc<MachineMode::IE>(MachineMode::TI);
    csrs<MachineMode::IP>(SupervisorMode::TI);
  }

  static void sbi(ContextFrame *context) {
    CLINT::reset();
    context->pc += 4;
  }

  static void fpu(ContextFrame *context) {
    if ((context->value & MachineMode::PP) != MachineMode::PP_M) {
      if (Decoder::floating(context->value)) {
        FPU::enable<MachineMode>(context);
        return;
      }
    }
    ExceptionHandler::esr(context);
  }
};

} // namespace QUARK
