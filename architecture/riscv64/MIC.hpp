#pragma once

#include <Traits.hpp>
#include <abi/Handler.hpp>
#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/Decoder.hpp>
#include <architecture/riscv64/FPU.hpp>
#include <architecture/riscv64/IC.hpp>
#include <architecture/riscv64/PLIC.hpp>
#include <architecture/riscv64/TrapHandler.hpp>

namespace QUARK {

class MIC {
    static constexpr bool IsMachineMode                 = !Traits<RISCV>::Supervisor;
    static constexpr bool IsTimerEnable                 = Traits<QUARK::Timer>::Enable;
    static constexpr bool IsExternalInterruptionsEnable = Traits<PLIC>::Enable;
    static constexpr bool ChangeStack = (IsMachineMode && Traits<Thread>::UserStack) || !IsMachineMode;
    using MachineContextHandler       = MachineContext<ChangeStack>;

  protected:
    static void forward(ContextFrame *) { CLINT::forward(); }

    static void syscall(ContextFrame *context) {
        CLINT::syscall();
        context->pc += 4;
    }

    static void fpu(ContextFrame *context) {
        if (Decoder::fp(context->value)) {
            FPU::enable<MachineMode>(context);
        }
    }

  public:
    static void init() {
        TrapHandler::init<MachineMode, ChangeStack>();

        if constexpr (!IsMachineMode) {
            CoreContextHandler<MachineMode>::stack(__amm.end() - Traits<Memory>::PageSize * CPU::id<true>());
            if constexpr (IsTimerEnable) {
                csrs<MachineMode::IP>(SupervisorMode::TI);
                TrapHandler::install(7, forward);
                TrapHandler::install(9, syscall, TrapHandler::Exception);
            }
            return;
        }

        if constexpr (IsExternalInterruptionsEnable) {
            PLIC::init();
            TrapHandler::install(11, IC::onTrap);
            csrs<MachineMode::IE>(MachineMode::EI);
        }

        // TrapHandler::install(2, fpu, TrapHandler::Exception);
        // TrapHandler::install(11, abi, TrapHandler::Exception);
    }

  private:
    // static void abi(ContextFrame *c) {
    //     c->a0 = reinterpret_cast<uintptr_t>(ABI::Handler::dispatch(static_cast<ABI::Function>(c->a7), &c->a0));
    //     c->pc += 4;
    // }
};

} // namespace QUARK
