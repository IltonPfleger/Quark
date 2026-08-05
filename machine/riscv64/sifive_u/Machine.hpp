#pragma once

#include <Traits.hpp>
#include <architecture/riscv64/init.hpp>
#include <drivers/uart/SiFiveUART.hpp>

namespace QUARK {

class Machine {
  public:
    static void init() {
        riscv64::init();
        Meta::forEach(Traits<UART>::Devices{}, []<typename T>() { T::init(); });
    }
    static void shutdown() { CPU::halt(); }
};

} // namespace QUARK
