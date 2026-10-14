#ifndef QUARK_ABI_CONSOLE_HPP
#define QUARK_ABI_CONSOLE_HPP

#include <abi/ABI.hpp>
#include <architecture/Syscall.hpp>
#include <utility/Console.hpp>
#include <utility/Formatter.hpp>

namespace QUARK::ABI {

class ConsoleHandler : public Formatter<ConsoleHandler> {
public:
  static void write(char character) {
    Syscall<void>(ABI::WRITE, 0, &character, 1);
  }
};

using Console = Meta::IF<Traits<Kernel>::Mode == Traits<Kernel>::KERNEL,
                         ConsoleHandler, QUARK::Console>::Result;

} // namespace QUARK::ABI

#endif
