#pragma once

#include <Meta.hpp>

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

template <typename> class SiFiveUART;

template <> struct Traits<Machine> {
    static constexpr const char NAME[] = "sifive_u";
};

template <> struct Traits<CPU> {
    static constexpr const char Architecture[] = "riscv64";
    static constexpr int Count                 = 5;
    static constexpr int Active                = Count - 1;
    static constexpr int Offset                = 1;
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

    static constexpr unsigned long RamStart  = Traits<Kernel>::Multitask ? VirtualRamStart : PhysicalRamStart;
    static constexpr unsigned long RamEnd    = Traits<Kernel>::Multitask ? VirtualRamEnd : PhysicalRamEnd;
    static constexpr unsigned long BootStart = RamStart;

    static constexpr unsigned long KernelAddress = RamStart;

    static constexpr unsigned long MMIO  = 0x00000000;
    static constexpr unsigned long UART0 = 0x10010000;
    static constexpr unsigned long CLINT = 0x02000000;
    static constexpr unsigned long PLIC  = 0xc000000;
};

template <> struct Traits<SiFiveUART<UART0>> {
    static constexpr unsigned long Address = Traits<MemoryMap>::UART0;
    static constexpr unsigned int Clock    = 10'000'000;
    static constexpr unsigned int BaudRate = 115200;
    static constexpr unsigned int IRQs[]   = {15};
};

template <> struct Traits<UART> {
    typedef Meta::TypeList<SiFiveUART<UART0>> Devices;
    static constexpr unsigned int NumberOfDevices = Devices::Length;
};

template <> struct Traits<CLINT> {
    static constexpr bool Enable           = Traits<Timer>::Enable;
    static constexpr unsigned long Address = Traits<MemoryMap>::CLINT;
    static constexpr unsigned long Clock   = 10'000'000;
};

template <> struct Traits<IC> {
    static constexpr unsigned long First = 0;
    static constexpr unsigned long Last  = 10 + 11;
};

template <> struct Traits<PLIC> {
    static constexpr bool Enable               = true;
    static constexpr int NumberOfInterruptions = 30;
    static constexpr int Contexts[5][2]        = {
        {0, -1}, // Hart 0: M->0, S->N/A
        {1, 2},  // Hart 1: M->1, S->2
        {3, 4},  // Hart 2: M->3, S->4
        {5, 6},  // Hart 3: M->5, S->6
        {7, 8}   // Hart 4: M->7, S->8
    };
};

} // namespace QUARK
