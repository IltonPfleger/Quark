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
    static constexpr bool IsMachineMode = !Traits<RISCV>::Supervisor;
    static constexpr bool IsTimerEnable = Traits<QUARK::Timer>::Enable;
    static constexpr bool ChangeStack   = (IsMachineMode && Traits<Thread>::UserStack) || !IsMachineMode;
    using MachineContextHandler         = MachineContext<ChangeStack>;

  public:
    static void init() {
        TrapHandler::init<MachineMode, ChangeStack>();

        if constexpr (Traits<RISCV>::Supervisor) {
            CoreContextHandler<MachineMode>::stack(__amm.end() - Traits<Memory>::PageSize * CPU::id<true>());

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

        if constexpr (Traits<Payload>::Unprivileged) {
            PMP::NAPOT<0>(0, 0, PMP::R | PMP::W | PMP::X);
            TrapHandler::install(8, abi, TrapHandler::Exception);
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

    static void abi(ContextFrame *c) {
        c->a0 = reinterpret_cast<uintptr_t>(ABI::Handler::dispatch(static_cast<ABI::Function>(c->a7), &c->a0));
        c->pc += 4;
    }

    static void fpu(ContextFrame *context) {
        if (Decoder::floating(context->value)) {
            FPU::enable<MachineMode>(context);
        } else {
            ExceptionHandler::onTrap(context);
        }
    }
};

} // namespace QUARK
