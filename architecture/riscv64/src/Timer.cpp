#include <architecture/CLINT.hpp>
#include <architecture/Timer.hpp>
#include <architecture/TrapHandler.hpp>
#include <architecture/VirtualCPU.hpp>

namespace QUARK {

Microsecond Timer::now() { return us(CLINT::mtime()); }

void Timer::init() {
  if constexpr (!Traits<RISCV>::Supervisor) {
    TrapHandler<MachineMode>::install(7, handler);
    csrs<MachineMode::IE>(MachineMode::TI);
    CLINT::write();
  } else {
    TrapHandler<SupervisorMode>::install(5, handler);
    csrs<SupervisorMode::IE>(SupervisorMode::TI);
  }
}

Microsecond Timer::us(uintmax_t ticks) {
  return Microsecond((ticks * 1'000'000ULL) / Traits<CLINT>::Clock);
}

void Timer::handler(ContextFrame *) {
  if constexpr (Traits<Application>::Virtualization) {
    VirtualCPU::update();
    CLINT::write();
  } else if (!Traits<RISCV>::Supervisor) {
    CLINT::write();
  } else {
    CPU::syscall();
  }
  Architecture::Timer::handler(CPU::id());
}

} // namespace QUARK
