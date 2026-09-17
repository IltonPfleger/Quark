#include <Thread.hpp>
#include <architecture/CPU.hpp>
#include <machine/UART.hpp>
#include <utility/Console.hpp>

namespace QUARK {

void Console::panic() { CPU::Atomic::cas(panic_, 0, CPU::id()); }

bool Console::panicked() { return (panic_ && panic_ != CPU::id()); }

void Console::write(char c) {
  if (panicked())
    return;

  Device::instance()->write(c);
}

} // namespace QUARK
