#pragma once

#include <Meta.hpp>
#include <Traits.hpp>

namespace QUARK {

class Machine;
class MemoryMap;
class Memory;
class CPU;
class CLINT;
class PLIC;
class UART;
class UART0;
class RISCV;
class IC;

template <typename> class UART16550;

template <> struct Traits<Machine> {
    static constexpr const char NAME[] = "virt";
};

template <> struct Traits<CPU> {
    static constexpr const char Architecture[] = "riscv64";
    static constexpr int Count                 = 5;
    static constexpr int Active                = Count;
    static constexpr int Offset                = 0;
    static constexpr int BSP                   = 0;
};

template <> struct Traits<Memory> {
    static constexpr unsigned Order     = 30;
    static constexpr unsigned Size      = (1 << Order);
    static constexpr unsigned PageSize  = 4096;
    static constexpr unsigned StackSize = PageSize;
};

template <> struct Traits<MemoryMap> {
    static constexpr unsigned long PhysicalRamStart = 0x80000000;
    static constexpr unsigned long PhysicalRamEnd   = PhysicalRamStart + Traits<Memory>::Size;

    static constexpr unsigned long VirtualRamStart = 0xffffffff80000000;
    static constexpr unsigned long VirtualRamEnd   = VirtualRamStart + Traits<Memory>::Size;

    static constexpr unsigned long RamStart = Traits<Kernel>::Multitask ? VirtualRamStart : PhysicalRamStart;
    static constexpr unsigned long RamEnd   = Traits<Kernel>::Multitask ? VirtualRamEnd : PhysicalRamEnd;

    static constexpr unsigned long BootStart     = RamStart;
    static constexpr unsigned long KernelAddress = RamStart;

    static constexpr unsigned long MMIO  = 0x00000000;
    static constexpr unsigned long UART0 = 0x10000000;
    static constexpr unsigned long CLINT = 0x02000000;
    static constexpr unsigned long PLIC  = 0xc000000;
};

template <> struct Traits<UART0> {
    static constexpr unsigned long Address = Traits<MemoryMap>::UART0;
    static constexpr unsigned int Clock    = 10'000'000;
    static constexpr unsigned int BaudRate = 115200;
    static constexpr unsigned int Shift    = 0;
    static constexpr unsigned int IRQs[]   = {10};
};

template <> struct Traits<UART> {
    typedef Meta::TypeList<UART16550<UART0>> Devices;
    static constexpr unsigned int NumberOfDevices = Devices::Length;
};

template <> struct Traits<CLINT> {
    static constexpr bool Enable           = Traits<Timer>::Enable;
    static constexpr unsigned long Address = Traits<MemoryMap>::CLINT;
    static constexpr unsigned long Clock   = 10'000'000;
};

template <> struct Traits<PLIC> {
    static constexpr bool Enable               = true;
    static constexpr int NumberOfInterruptions = 30;
    using ContextsType                         = Meta::Array<Traits<CPU>::Count, Meta::Array<2, int>>;
    static constexpr ContextsType Contexts     = []() {
        ContextsType contexts{};
        for (int i = 0; i < Traits<CPU>::Count; i++) {
            contexts[i][0] = i * 2;
            contexts[i][1] = i * 2 + 1;
        }
        return contexts;
    }();
};

} // namespace QUARK
