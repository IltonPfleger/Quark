#include <architecture/CPU.hpp>
#include <architecture/riscv64/init.hpp>
#include <machine/Machine.hpp>

using namespace QUARK;

extern "C" void init();

extern "C" __attribute__((optimize("O0"), naked, used, section(".init"))) void _init() {
    riscv64::init();
    init();
}
