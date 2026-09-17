#ifndef __QUARK_ABI_CONSOLE__
#define __QUARK_ABI_CONSOLE__

#include <abi/ABI.hpp>
#include <architecture/Syscall.hpp>
#include <utility/Console.hpp>
#include <utility/Printer.hpp>

namespace QUARK::ABI {

class ConsoleHandler : public Printer<ConsoleHandler> {
public:
  static void write(char character) {
    Syscall<void>(ABI::READ, 0, &character, 1);
  }
};

using Console = Meta::IF<Traits<Kernel>::Mode == Traits<Kernel>::KERNEL,
                         ConsoleHandler, QUARK::Console>::Result;

} // namespace QUARK::ABI

#endif
