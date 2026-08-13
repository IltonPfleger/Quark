#include <Thread.hpp>
#include <architecture/CPU.hpp>
#include <machine/UART.hpp>
#include <utility/Console.hpp>

namespace QUARK {

void Console::print(char c) {
    if (panicked()) return;

    Device *device = Device::instance();

    if (c == '\n') {
        device->write('\r');
    }

    device->write(c);
}

void Console::panic() { CPU::Atomic::cas(panic_, 0, CPU::id()); }

bool Console::panicked() { return (panic_ && panic_ != CPU::id()); }

} // namespace QUARK
