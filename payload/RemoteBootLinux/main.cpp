#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <architecture/VirtualCPU.hpp>
#include <architecture/VirtualPLIC.hpp>
#include <architecture/riscv64/sbi/Counter.hpp>
#include <hypervisor/GenericVirtualMachine.hpp>
#include <hypervisor/VirtualSwitch.hpp>
#include <hypervisor/dtb/FDT_Builder.hpp>
#include <hypervisor/virtio/Console.hpp>
#include <hypervisor/virtio/Network.hpp>
#include <machine/Machine.hpp>
#include <network/link/LinkIPv4ToEthernet.hpp>
#include <network/protocols/TFTP.hpp>
#include <utility/Span.hpp>

using namespace QUARK;

constexpr size_t MB = 1024 * 1024;

class Receiver {
public:
  Receiver(TFTP &tftp) : tftp_(tftp), buffer_(new uint8_t[BufferSize]) {
    const IPv4::Address server(192, 168, 1, 100);

    uint8_t *current = buffer_;
    size_t remaining = BufferSize;
    size_t size;

    size = tftp_.request(server, "RemoteBootVisionFive2Kernel", current,
                         remaining);
    new (&linux_) Span(current, size);
    current += size;
    remaining -= size;

    size = tftp_.request(server, "RemoteBootVisionFive2InitRD.cpio", current,
                         remaining);

    new (&initramfs_) Span(current, size);
    current += size;
    remaining -= size;
  }

  ~Receiver() { delete[] buffer_; }

  const auto &linux() const { return linux_; }
  const auto &initramfs() const { return initramfs_; }

private:
  static constexpr size_t BufferSize = 64 * 1024 * 1024;

private:
  TFTP &tftp_;
  uint8_t *const buffer_;
  Span<const uint8_t> linux_;
  Span<const uint8_t> initramfs_;
};

class LinuxLauncher {
public:
  static constexpr uint32_t CPUS = 4;

  using SerialDevice = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
  using Serial = virtio::Console<SerialDevice, 0x30000000, 32>;
  using InterruptController = VirtualPLIC<CPUS, 0xc000000>;
  using LinuxMachine = GenericVirtualMachine<CPUS, Serial, InterruptController>;

  LinuxLauncher(size_t size, Span<const uint8_t> kernel,
                Span<const uint8_t> initramfs, size_t offset)
      : size_(size), start_(nullptr), initramfs_(initramfs), dtb_(nullptr) {

    start_ = static_cast<unsigned char *>(Memory::alloc(size_));

    unsigned char *current = start_;

    memcpy(current, kernel, kernel.length());
    current += kernel.length();
    current += 32 * MB;

    memcpy(current, initramfs, initramfs.length());
    initramfs_ = Span<const uint8_t>(current, initramfs.length());
    current += initramfs.length();

    current = align(current, 8);
    dtb(current, size_ - initramfs.length() - kernel.length());

    Console::println("\n *** Linux is at core ", CPU::id(), " ***");

    (new LinuxMachine(start_, size_, offset))->boot(0, start_, dtb_);
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
        fdt.add("bootargs", "console=hvc0 loglevel=8 earlycon=sbi");

        // fdt.add("bootargs",
        //        "console=hvc0 loglevel=8 earlycon=sbi initcall_debug debug");

        uint64_t start = reinterpret_cast<uint64_t>(initramfs_.data());
        uint64_t end = start + initramfs_.length();
        uint32_t regs0[] = {CPU::hi32(start), CPU::lo32(start)};
        uint32_t regs1[] = {CPU::hi32(end), CPU::lo32(end)};
        fdt.add("linux,initrd-start", regs0, 2);
        fdt.add("linux,initrd-end", regs1, 2);
      }
      fdt.end();

      fdt.begin("cpus");
      {
        fdt.add("#address-cells", 1);
        fdt.add("#size-cells", 0u);
        fdt.add("timebase-frequency", 4000000);

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
};

int main() {
  using namespace QUARK;

  typedef Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result Device;

  Device::init();

  auto *link = new QUARK::LinkIPv4ToEthernet(Device::instance());
  auto *ipv4 = new QUARK::IPv4(IPv4::Address(192, 168, 1, 101), *link);
  auto *udp = new QUARK::UDP(*ipv4);
  auto *tftp = new QUARK::TFTP(*udp);
  auto *receiver = new Receiver(*tftp);

  const size_t MemorySize = 1024 * 1024 * 128;

  new LinuxLauncher(MemorySize, receiver->linux(), receiver->initramfs(), 0);

  return 0;
}
