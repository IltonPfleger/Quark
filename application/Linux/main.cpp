#include <Thread.hpp>
#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <architecture/VirtualPLIC.hpp>
#include <hypervisor/GenericVirtualMachine.hpp>
#include <hypervisor/fdt/FDT_Builder.hpp>
#include <hypervisor/virtio/Console.hpp>
#include <machine/Machine.hpp>
#include <utility/Console.hpp>
#include <utility/Delay.hpp>
#include <utility/Span.hpp>

using namespace QUARK;

constexpr size_t MB = 1024 * 1024;

__attribute__((section(".__linux__"), used)) static uint8_t __guest[32 * MB];
__attribute__((section(".__initrd__"), used)) static uint8_t __initrd[16 * MB];

class LinuxFlattenedDeviceTree {
  template <typename> struct VirtioDeviceParser;
  template <template <typename, auto, auto> class D, typename M, auto A, auto I>
  struct VirtioDeviceParser<D<M, A, I>> {
    static constexpr auto Address = A;
    static constexpr auto IRQ = I;
  };

public:
  LinuxFlattenedDeviceTree(void *buffer, size_t capacity)
      : builder_(buffer, capacity) {}

  void begin() {
    builder_.begin("");
    builder_.add("#address-cells", 2);
    builder_.add("#size-cells", 2);
    builder_.add("compatible", "riscv-virtio");
    builder_.add("model", "riscv-virtio,qemu");
  }

  void chosen(Span<const uint8_t> initrd) {
    builder_.begin("chosen");
    builder_.add("bootargs", "console=hvc0 loglevel=8 earlycon=sbi");

    uint64_t start = reinterpret_cast<uint64_t>(initrd.data());
    uint64_t end = start + initrd.length();

    uint32_t regs0[] = {CPU::hi32(start), CPU::lo32(start)};
    uint32_t regs1[] = {CPU::hi32(end), CPU::lo32(end)};

    builder_.add("linux,initrd-start", regs0, 2);
    builder_.add("linux,initrd-end", regs1, 2);

    builder_.end();
  }

  template <size_t CPUS> void cpus() {
    builder_.begin("cpus");
    builder_.add("#address-cells", 1);
    builder_.add("#size-cells", 0u);
    builder_.add("timebase-frequency", 4000000);

    for (uint32_t core = 0; core < CPUS; ++core) {
      char name[16] = "cpu@";
      size_t length = 4;

      if (core == 0) {
        name[length++] = '0';
      } else {
        char temporary[10];
        uint32_t digits = 0;
        uint32_t value = core;

        while (value) {
          temporary[digits++] = '0' + (value % 10);
          value /= 10;
        }

        while (digits)
          name[length++] = temporary[--digits];
      }

      name[length] = '\0';

      builder_.begin(name);
      builder_.add("device_type", "cpu");
      builder_.add("reg", core);
      builder_.add("status", "okay");
      builder_.add("compatible", "riscv");
      builder_.add("riscv,isa", "rv64imafdcsu");
      builder_.add("mmu-type", "riscv,sv39");

      builder_.begin("interrupt-controller");
      builder_.add("#interrupt-cells", 1);
      builder_.add("interrupt-controller");
      builder_.add("compatible", "riscv,cpu-intc");
      builder_.add("phandle", 0x10 + core);
      builder_.end();

      builder_.end();
    }

    builder_.end();
  }

  void memory(Span<const uint8_t> memory) {
    builder_.begin("memory");
    builder_.add("device_type", "memory");

    uintptr_t address = memory.pointer();
    uintptr_t size = memory.length();

    uint32_t regs[] = {CPU::hi32(address), CPU::lo32(address), CPU::hi32(size),
                       CPU::lo32(size)};

    builder_.add("reg", regs, 4);
    builder_.end();
  }

  template <typename PLIC, typename... IO> void soc() {
    builder_.begin("soc");
    builder_.add("#address-cells", 2);
    builder_.add("#size-cells", 2);
    builder_.add("compatible", "simple-bus");
    builder_.add("ranges");

    plic(static_cast<const PLIC *>(nullptr));
    devices<IO...>();

    builder_.end();
  }

  void end() {
    builder_.end();
    builder_.finish();
  }

private:
  template <template <auto, auto> class P, size_t C, uintptr_t A>
  void plic(const P<C, A> *) {
    builder_.begin("interrupt-controller@c000000");

    builder_.add("compatible", "riscv,plic0");

    uint32_t regs[] = {0x00, A, 0x00, 0x4000000};
    builder_.add("reg", regs, 4);

    builder_.add("interrupt-controller");
    builder_.add("#interrupt-cells", 1);
    builder_.add("riscv,ndev", 0x35);

    uint32_t interrupts[C * 2];

    for (uint32_t core = 0; core < C; ++core) {
      interrupts[core * 2] = 0x10 + core;
      interrupts[core * 2 + 1] = 9;
    }

    builder_.add("interrupts-extended", interrupts, C * 2);
    builder_.add("phandle", 0x02);
    builder_.end();
  }

  template <typename... IO> void devices() {
    (virtio(static_cast<const IO *>(nullptr)), ...);
  }

  template <typename D> void virtio(const D *) {
    constexpr auto A = VirtioDeviceParser<D>::Address;
    constexpr auto I = VirtioDeviceParser<D>::IRQ;
    char name[24] = "virtio@";
    constexpr char hex[] = "0123456789abcdef";

    for (size_t i = 0; i < 16; ++i)
      name[7 + i] = hex[(A >> ((15 - i) * 4)) & 0xf];

    name[23] = '\0';

    uint32_t regs[] = {CPU::hi32(A), CPU::lo32(A), 0x00, 0x1000};

    builder_.begin(name);
    builder_.add("compatible", "virtio,mmio");
    builder_.add("reg", regs, 4);
    builder_.add("interrupts", I);
    builder_.add("interrupt-parent", 0x02);
    builder_.end();
  }

private:
  FDT_Builder builder_;
};

template <size_t CPUS, typename... IO> class LinuxLauncher {
public:
  using PLIC = VirtualPLIC<CPUS, 0xc000000>;
  using ExternalDevices = Meta::Pack<IO...>;
  using VirtualMachine = GenericVirtualMachine<CPUS, PLIC, IO...>;

  LinuxLauncher(size_t size, Span<const uint8_t> kernel,
                Span<const uint8_t> initrd, size_t offset)
      : size_(size), start_(static_cast<uint8_t *>(Memory::alloc(size_))),
        vm_(start_, size_, offset) {

    uint8_t *end = start_ + size_;
    uint8_t *current = start_;

    memcpy(current, kernel, kernel.length());
    current += kernel.length();
    current = align(current, 8);
    current += 32 * MB;

    const uint8_t *address = current;
    memcpy(current, initrd, initrd.length());
    current += initrd.length();

    current = align(current, 8);

    size_t remaining = size_ - (current - start_);

    void *opaque = fdt(current, remaining, Span(address, initrd.length()));

    vm_.boot(0, start_, opaque);
  }

  static unsigned char *align(unsigned char *pointer, long alignment) {
    uintptr_t address = reinterpret_cast<long>(pointer);
    address = (address + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<unsigned char *>(address);
  }

  void *fdt(void *buffer, size_t capacity, Span<const uint8_t> initrd) {
    LinuxFlattenedDeviceTree fdt(buffer, capacity);
    fdt.begin();
    fdt.chosen(initrd);
    fdt.cpus<CPUS>();
    fdt.memory(Span(const_cast<const uint8_t *>(start_), size_));
    fdt.soc<PLIC, IO...>();
    fdt.end();
    return buffer;
  }

private:
  size_t size_;
  uint8_t *start_;
  VirtualMachine vm_;
};

int main() {
  Span<const uint8_t> kernel(__guest, sizeof(__guest));
  Span<const uint8_t> initramfs(__initrd, sizeof(__initrd));
  using pUART = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
  using vUART = virtio::Console<pUART, 0x30000000, 32>;
  using Launcher = LinuxLauncher<Traits<CPU>::Active, vUART>;
  Launcher launcher(128 * MB, kernel, initramfs, 0);
  Delay delay(4'000'000);
  return 0;
}
