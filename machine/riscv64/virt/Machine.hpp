#ifndef __QUARK_MACHINE_RISCV64_VIRT__
#define __QUARK_MACHINE_RISCV64_VIRT__

#include <Traits.hpp>
#include <machine/UART.hpp>

namespace QUARK {

class Machine {
public:
  static void init() {
    auto initializer = []<typename T>() { return T::init(); };
    Meta::forEach(Traits<UART>::Devices{}, initializer);
  }

  static void shutdown() { CPU::halt(); }
};

} // namespace QUARK

#endif
