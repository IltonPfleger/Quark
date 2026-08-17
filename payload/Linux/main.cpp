#include <Thread.hpp>
#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <architecture/riscv64/VirtualPLIC.hpp>
#include <hypervisor/GenericVirtualMachine.hpp>
#include <hypervisor/dtb/FDT_Builder.hpp>
#include <hypervisor/virtio/Console.hpp>
#include <machine/Machine.hpp>
#include <utility/Console.hpp>
#include <utility/Delay.hpp>
#include <utility/Span.hpp>

using namespace QUARK;

constexpr size_t MB = 1024 * 1024;

__attribute__((section(".__linux__"), used)) static uint8_t LINUX[32 * MB];
__attribute__((section(".__initrd__"), used)) static uint8_t INITRD[8 * MB];

class LinuxLauncher {
public:
  static constexpr uint32_t CPUS = 1;

  using SerialDevice = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
  using Serial = virtio::Console<SerialDevice, 0x30000000, 32>;
  using InterruptController = VirtualPLIC<CPUS, 0xc000000>;
  using LinuxMachine = GenericVirtualMachine<CPUS, Serial, InterruptController>;

  LinuxLauncher(size_t size, Span<const uint8_t> kernel,
                Span<const uint8_t> initramfs)
      : size_(size), start_(nullptr), initramfs_(initramfs), dtb_(nullptr),
        vm_(nullptr) {

    start_ = static_cast<unsigned char *>(Memory::alloc(size_));

    unsigned char *current = start_;

    memcpy(current, kernel, kernel.length());
    current += kernel.length();

    current = align(current, 4 * MB);
    memcpy(current, initramfs, initramfs.length());
    initramfs_ = Span<const uint8_t>(current, initramfs.length());
    current += initramfs.length();

    dtb(current, size_ - initramfs.length() - kernel.length());

    Console::println("*** Linux is at core ", CPU::id(), " ***");

    (new LinuxMachine(start_, size_, 0))->boot(0, start_, dtb_);
  }

  static unsigned char *align(unsigned char *pointer, long alignment) {
    uintptr_t address = reinterpret_cast<long>(pointer);
    address = (address + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<unsigned char *>(address);
  }

  size_t dtb(void *buffer, size_t capacity) {
    FDT_Builder fdt(buffer, capacity);

    dtb_ = static_cast<unsigned char *>(buffer);
    uint64_t base = reinterpret_cast<uint64_t>(start_);
    fdt.begin("");
    {
      fdt.add("#address-cells", 2);
      fdt.add("#size-cells", 2);
      fdt.add("compatible", "riscv-virtio");
      fdt.add("model", "riscv-virtio,qemu");

      fdt.begin("chosen");
      {
        uint64_t start = reinterpret_cast<uint64_t>(initramfs_.data());
        uint64_t end = start + initramfs_.length();
        fdt.add("bootargs", "console=hvc0 loglevel=8");
        uint32_t regs0[] = {CPU::hi32(start), CPU::lo32(start)};
        fdt.add("linux,initrd-start", regs0, 2);
        uint32_t regs1[] = {CPU::hi32(end), CPU::lo32(end)};
        fdt.add("linux,initrd-end", regs1, 2);
      }
      fdt.end();

      fdt.begin("cpus");
      {
        fdt.add("#address-cells", 1);
        fdt.add("#size-cells", 0u);
        fdt.add("timebase-frequency", 10000000);

        for (uint32_t core = 0; core < CPUS; core++) {
          char name[16];
          size_t length = 4;
          name[0] = 'c';
          name[1] = 'p';
          name[2] = 'u';
          name[3] = '@';

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

            while (digits) {
              name[length++] = temporary[--digits];
            }
          }

          name[length] = '\0';

          fdt.begin(name);
          {
            fdt.add("device_type", "cpu");
            fdt.add("reg", core);
            fdt.add("status", "okay");
            fdt.add("compatible", "riscv");
            fdt.add("riscv,isa", "rv64imafdcsu");
            fdt.add("mmu-type", "riscv,sv39");

            fdt.begin("interrupt-controller");
            {
              fdt.add("#interrupt-cells", 1);
              fdt.add("interrupt-controller");
              fdt.add("compatible", "riscv,cpu-intc");
              fdt.add("phandle", 0x10 + core);
            }
            fdt.end();
          }
          fdt.end();
        }
      }
      fdt.end();

      fdt.begin("memory");
      {
        fdt.add("device_type", "memory");
        uint32_t regs[] = {CPU::hi32(base), CPU::lo32(base), CPU::hi32(size_),
                           CPU::lo32(size_)};
        fdt.add("reg", regs, 4);
      }
      fdt.end();

      fdt.begin("soc");
      {
        fdt.add("#address-cells", 2);
        fdt.add("#size-cells", 2);
        fdt.add("compatible", "simple-bus");
        fdt.add("ranges");

        fdt.begin("interrupt-controller@c000000");
        {
          fdt.add("compatible", "sifive,plic-1.0.0");

          uint32_t regs0[] = {0x00, 0xc000000, 0x00, 0x4000000};
          fdt.add("reg", regs0, 4);

          fdt.add("interrupt-controller");
          fdt.add("#interrupt-cells", 1);
          fdt.add("riscv,ndev", 0x35);

          uint32_t plic[CPUS * 4];
          for (uint32_t core = 0; core < CPUS; core++) {
            uint32_t phandle = 0x10 + core;
            plic[core * 4 + 0] = phandle;
            plic[core * 4 + 1] = 0x0b;
            plic[core * 4 + 2] = phandle;
            plic[core * 4 + 3] = 0x09;
          }
          fdt.add("interrupts-extended", plic, CPUS * 4);
          fdt.add("phandle", 0x02);
        }
        fdt.end();

        fdt.begin("virtio@30000000");
        {
          uint64_t address = 0x30000000;
          uint32_t irq = 32;
          uint32_t regs[] = {CPU::hi32(address), CPU::lo32(address), 0x00,
                             0x1000};
          fdt.add("compatible", "virtio,mmio");
          fdt.add("reg", regs, 4);
          fdt.add("interrupts", irq);
          fdt.add("interrupt-parent", 0x02);
        }
        fdt.end();
      }
      fdt.end();
    }
    fdt.end();

    return fdt.finish();
  }

private:
  size_t size_;
  unsigned char *start_;
  Span<const uint8_t> initramfs_;
  unsigned char *dtb_;
  LinuxMachine *vm_;
};

int main() {
  TraceIn();

  Span<const uint8_t> kernel(LINUX, sizeof(LINUX));
  Span<const uint8_t> initramfs(INITRD, sizeof(INITRD));

  new LinuxLauncher(128 * 1024 * 1024, kernel, initramfs);

  return 0;
}
