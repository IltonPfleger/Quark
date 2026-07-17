#pragma once

#include <Traits.hpp>
#include <architecture/common/Timer.hpp>
#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/TrapHandler.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>

namespace QUARK {

class Timer : public ArchitectureCommon::Timer {
  public:
    // static Microsecond now() { return us(CLINT::mtime()); }
    static Nanosecond now() { return ns(CLINT::mtime()); }

    class Delay {
      public:
        template <typename T> Delay(T delta) {
            Microsecond elapsed = now() + delta;
            while (now() < elapsed)
                ;
        }
    };

    static void init() {
        if constexpr (!Traits<RISCV>::Supervisor) {
            TrapHandler::install(7, dispatch);
            csrs<MachineMode::IE>(MachineMode::TI);
            CLINT::write();
        } else {
            TrapHandler::install(5, dispatch);
            csrs<SupervisorMode::IE>(SupervisorMode::TI);
        }
    }

  private:
    static Microsecond us(uintmax_t ticks) { return Microsecond((ticks * 1'000'000ULL) / Traits<CLINT>::Clock); }
    static Nanosecond ns(uintmax_t ticks) { return Nanosecond((ticks * 1'000'000'000ULL) / Traits<CLINT>::Clock); }

    static void dispatch(ContextFrame *) {
        if constexpr (Traits<Payload>::Virtualized) {
            CLINT::write();
            VirtualCPU::onTick();
        } else if (!Traits<RISCV>::Supervisor) {
            CLINT::write();
        } else {
            CPU::syscall();
        }
        ArchitectureCommon::Timer::handler(CPU::id());
    }
};

} // namespace QUARK
