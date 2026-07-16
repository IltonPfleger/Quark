#pragma once

#include <Traits.hpp>

namespace QUARK {

class MemoryMap;
class Memory;
class Clock;
class CPU;
class CLINT;
class PLIC;
class UART;
class UART0;
class GMAC0;
class IPv4;
class IC;
class CAN0;
class PMIC0;
class PMIC;
class I2C;
class I2C5;
class DVFS;
class CacheController0;
class CacheController;
class Ethernet;

template <typename> class DesignWare_I2C_Controller;
template <typename> class AXP15060_Controller;
template <typename> class SiFiveU74_L2_CacheController;
template <typename> class UART16550;
template <typename> class DWC_Ether_QoS;
template <typename> class IPMS_CANFD;

template <> struct Traits<Machine> {
    static constexpr const char NAME[] = "VisionFive2";
};

template <> struct Traits<CPU> {
    static constexpr const char Architecture[] = "riscv64";
    static constexpr int Count                 = 5;
    static constexpr int Active                = Count - 1;
    static constexpr int Offset                = 1;
    static constexpr int BSP                   = 0;
};

template <> struct Traits<Memory> {
    static constexpr unsigned long Order    = 30;
    static constexpr unsigned long Size     = (1 << Order);
    static constexpr unsigned int PageSize  = 4096;
    static constexpr unsigned int StackSize = PageSize;
};

template <> struct Traits<MemoryMap> {
    static constexpr unsigned long PhysicalRamStart = 0x40000000;
    static constexpr unsigned long PhysicalRamEnd   = PhysicalRamStart + Traits<Memory>::Size;

    static constexpr unsigned long VirtualRamStart = 0xffffffff80000000;
    static constexpr unsigned long VirtualRamEnd   = VirtualRamStart + Traits<Memory>::Size;

    static constexpr unsigned long RamStart  = Traits<Kernel>::Multitask ? VirtualRamStart : PhysicalRamStart;
    static constexpr unsigned long RamEnd    = Traits<Kernel>::Multitask ? VirtualRamEnd : PhysicalRamEnd;
    static constexpr unsigned long BootStart = RamStart;

    static constexpr unsigned long KernelAddress = RamStart;

    /* *** MMIO *** */
    static constexpr unsigned long MMIO       = 0x00000000;
    static constexpr unsigned long GMAC0      = 0x16030000;
    static constexpr unsigned long GMAC1      = 0x16040000;
    static constexpr unsigned long UART0      = 0x10000000;
    static constexpr unsigned long CAN0       = 0x130D0000;
    static constexpr unsigned long I2C5       = 0x12050000;
    static constexpr unsigned long CLINT      = 0x2000000;
    static constexpr unsigned long PLIC       = 0x0C000000;
    static constexpr unsigned long SYSCRG     = 0x13020000;
    static constexpr unsigned long AONCRG     = 0x17000000;
    static constexpr unsigned long SYS_SYSCON = 0x13030000;
    static constexpr unsigned long AON_SYSCON = 0x17010000;
};

/* ********** CacheController ********** */
template <> struct Traits<CacheController0> {
    static constexpr size_t Masters                  = 30;
    static constexpr size_t CacheSize                = 2 * 1024 * 1024;
    static constexpr size_t NumberOfWays             = 16;
    static constexpr size_t CacheLineSize            = 64;
    static constexpr bool Isolation                  = false;
    static constexpr bool Prefetcher                 = false;
    static constexpr bool Enable                     = true;
    static constexpr uintptr_t PrefetcherAddresses[] = {0x2032000, 0x2034000, 0x2036000, 0x2038000};
};

template <> struct Traits<CacheController> {
    typedef Meta::TypeList<SiFiveU74_L2_CacheController<CacheController0>> Devices;
    static constexpr unsigned int NumberOfDevices = Devices::Length;
};

/* ********** CLINT ********** */
template <> struct Traits<CLINT> {
    static constexpr bool Enable           = Traits<Timer>::Enable;
    static constexpr unsigned long Address = Traits<MemoryMap>::CLINT;
    static constexpr unsigned long Clock   = 4'000'000;
};

/* ********** PLIC ********** */
template <> struct Traits<PLIC> {
    static constexpr bool Enable               = true;
    static constexpr int NumberOfInterruptions = 127;
    static constexpr int Contexts[5][2]        = {
        {0, -1}, // Hart 0: M->0, S->N/A
        {1, 2},  // Hart 1: M->1, S->2
        {3, 4},  // Hart 2: M->3, S->4
        {5, 6},  // Hart 3: M->5, S->6
        {7, 8}   // Hart 4: M->7, S->8
    };
};

/* ********** UART ********** */
template <> struct Traits<UART0> {
    static constexpr unsigned long Address = Traits<MemoryMap>::UART0;
    static constexpr unsigned int Clock    = 24'000'000;
    static constexpr unsigned int BaudRate = 115200;
    static constexpr unsigned int Shift    = 2;
    static constexpr unsigned int IRQs[]   = {32};
};

template <> struct Traits<UART> {
    typedef Meta::TypeList<UART16550<UART0>> Devices;
    static constexpr unsigned int NumberOfDevices = Devices::Length;
};

/* ********** Ethernet ********** */
template <> struct Traits<GMAC0> {
    static constexpr uintptr_t Address         = Traits<MemoryMap>::GMAC0;
    static constexpr size_t SendBufferCount    = 10;
    static constexpr size_t ReceiveBufferCount = 10;
    static constexpr unsigned char MAC[]       = {12, 34, 56, 78, 12, 34};
    static constexpr unsigned int IRQs[]       = {9};
};

template <> struct Traits<Ethernet> {
    typedef Meta::TypeList<DWC_Ether_QoS<GMAC0>> Devices;
    static constexpr unsigned int NumberOfDevices = Devices::Length;
};

/* ********** CAN ********** */
template <> struct Traits<CAN0> {
    static constexpr unsigned long Address = Traits<MemoryMap>::CAN0;
    static constexpr unsigned int IRQs[]   = {112, 115};
};

/* ********** I2C ********** */
template <> struct Traits<I2C5> {
    static constexpr unsigned long Address = Traits<MemoryMap>::I2C5;
};

template <> struct Traits<I2C> {
    typedef Meta::TypeList<DesignWare_I2C_Controller<I2C5>> Devices;
    static constexpr unsigned int NumberOfDevices = Devices::Length;
};

/* ********** PMIC ********** */
template <> struct Traits<PMIC0> {
    static constexpr unsigned long Address      = 0x36;
    static constexpr unsigned int Voltages[][3] = {{1, 800000, 1040000}};
};

template <> struct Traits<PMIC> {
    typedef Meta::TypeList<AXP15060_Controller<PMIC0>> Devices;
    static constexpr unsigned int NumberOfDevices = Devices::Length;
};

} // namespace QUARK
