#ifndef __QUARK_MACHINE_RISCV64_VIRT_TRAITS__
#define __QUARK_MACHINE_RISCV64_VIRT_TRAITS__

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
class PMU;
class FPU;

template <typename> class NS16550;

template <> struct Traits<Machine> {
  static constexpr const char NAME[] = "virt";
};

template <> struct Traits<CPU> {
  static constexpr const char Architecture[] = "riscv64";
  static constexpr int Count = 5;
  static constexpr int Active = Count;
  static constexpr int Offset = 0;
  static constexpr int BSP = 0;
};

template <> struct Traits<Memory> {
  static constexpr unsigned Order = 30;
  static constexpr unsigned Size = (1 << Order);
  static constexpr unsigned PageSize = 4096;
  static constexpr unsigned StackSize = PageSize * 4;
};

template <> struct Traits<MemoryMap> {
  static constexpr bool HigherMapping =
      Traits<Kernel>::Mode == Traits<Kernel>::KERNEL;

  static constexpr unsigned long PhysicalRamStart = 0x80000000;

  static constexpr unsigned long PhysicalRamEnd =
      PhysicalRamStart + Traits<Memory>::Size;

  static constexpr unsigned long VirtualRamStart = 0xffffffff80000000;

  static constexpr unsigned long VirtualRamEnd =
      VirtualRamStart + Traits<Memory>::Size;

  static constexpr unsigned long RamStart =
      HigherMapping ? VirtualRamStart : PhysicalRamStart;

  static constexpr unsigned long RamEnd =
      HigherMapping ? VirtualRamEnd : PhysicalRamEnd;

  static constexpr unsigned long Boot = RamStart;

  static constexpr unsigned long Application =
      HigherMapping ? 0x800000000 : (RamStart + Traits<Memory>::Size / 2);

  static constexpr unsigned long MMIO = 0x00000000;
  static constexpr unsigned long UART0 = 0x10000000;
  static constexpr unsigned long CLINT = 0x02000000;
  static constexpr unsigned long PLIC = 0xc000000;
};

template <> struct Traits<UART0> {
  static constexpr unsigned long Address = Traits<MemoryMap>::UART0;
  static constexpr unsigned int Clock = 10'000'000;
  static constexpr unsigned int BaudRate = 115200;
  static constexpr unsigned int Shift = 0;
  static constexpr unsigned int IRQs[] = {10};
};

template <> struct Traits<UART> {
  typedef Meta::TypeList<NS16550<UART0>> Devices;
  static constexpr unsigned int NumberOfDevices = Devices::Length;
};

template <> struct Traits<CLINT> {
  static constexpr bool Enable = Traits<Timer>::Enable;
  static constexpr unsigned long Address = Traits<MemoryMap>::CLINT;
  static constexpr unsigned long Clock = 10'000'000;
};

template <> struct Traits<PLIC> {
  static constexpr bool Enable = true;
  static constexpr int NumberOfInterruptions = 30;
  using Array = Meta::Array<Traits<CPU>::Count, Meta::Array<2, int>>;
  static constexpr Array Contexts = []() {
    Array contexts{};
    for (int i = 0; i < Traits<CPU>::Count; i++) {
      contexts[i][0] = i * 2;
      contexts[i][1] = i * 2 + 1;
    }
    return contexts;
  }();
};

template <> struct Traits<FPU> {
  static constexpr bool Enable = false;
};

} // namespace QUARK

#endif
