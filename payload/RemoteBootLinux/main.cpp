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

    linux_ = Span<const uint8_t>(current, size);
    current += size;
    remaining -= size;

    size = tftp_.request(server, "RemoteBootVisionFive2InitRD.cpio", current,
                         remaining);

    initramfs_ = Span<const uint8_t>(current, size);
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

  // using LinuxMachine = GenericVirtualMachine<CPUS, InterruptController>;

  LinuxLauncher(size_t size, Span<const uint8_t> kernel,
                Span<const uint8_t> initrd, size_t offset)
      : size_(size), start_(nullptr) {

    start_ = static_cast<uint8_t *>(Memory::alloc(size_));
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

    void *opaque = dtb(current, remaining, Span(address, initrd.length()));

    Console::println("\n *** Linux is at core ", CPU::id(), " ***");

    LinuxMachine *vm = new LinuxMachine(start_, size_, offset);
    vm->boot(0, start_, opaque);
  }

  static unsigned char *align(unsigned char *pointer, long alignment) {
    uintptr_t address = reinterpret_cast<long>(pointer);
    address = (address + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<unsigned char *>(address);
  }

  void *dtb(void *buffer, size_t capacity, Span<const uint8_t> initrd) {
    FDT_Builder builder(buffer, capacity);

    uint64_t memory = reinterpret_cast<uint64_t>(start_);

    builder.begin("");
    {
      builder.add("#address-cells", 2);
      builder.add("#size-cells", 2);
      builder.add("compatible", "riscv-virtio");
      builder.add("model", "riscv-virtio,qemu");

      builder.begin("chosen");
      {
        builder.add("bootargs", "console=hvc0 loglevel=8 earlycon=sbi ");

        uint64_t start = reinterpret_cast<uint64_t>(initrd.data());
        uint64_t end = start + initrd.length();

        uint32_t regs0[] = {CPU::hi32(start), CPU::lo32(start)};
        uint32_t regs1[] = {CPU::hi32(end), CPU::lo32(end)};

        builder.add("linux,initrd-start", regs0, 2);
        builder.add("linux,initrd-end", regs1, 2);
      }
      builder.end();

      builder.begin("cpus");
      {
        builder.add("#address-cells", 1);
        builder.add("#size-cells", 0u);
        builder.add("timebase-frequency", 4000000);

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

          builder.begin(name);
          {
            builder.add("device_type", "cpu");
            builder.add("reg", core);
            builder.add("status", "okay");
            builder.add("compatible", "riscv");
            builder.add("riscv,isa", "rv64imafdcsu");
            builder.add("mmu-type", "riscv,sv39");

            builder.begin("interrupt-controller");
            {
              builder.add("#interrupt-cells", 1);
              builder.add("interrupt-controller");
              builder.add("compatible", "riscv,cpu-intc");
              builder.add("phandle", 0x10 + core);
            }
            builder.end();
          }
          builder.end();
        }
      }
      builder.end();

      builder.begin("memory");
      {
        builder.add("device_type", "memory");
        uint32_t regs[] = {CPU::hi32(memory), CPU::lo32(memory),
                           CPU::hi32(size_), CPU::lo32(size_)};
        builder.add("reg", regs, 4);
      }
      builder.end();

      builder.begin("soc");
      {
        builder.add("#address-cells", 2);
        builder.add("#size-cells", 2);
        builder.add("compatible", "simple-bus");
        builder.add("ranges");

        builder.begin("interrupt-controller@c000000");
        {
          builder.add("compatible", "riscv,plic0");

          uint32_t regs0[] = {0x00, 0xc000000, 0x00, 0x4000000};
          builder.add("reg", regs0, 4);

          builder.add("interrupt-controller");
          builder.add("#interrupt-cells", 1);
          builder.add("riscv,ndev", 0x35);

          uint32_t plic[CPUS * 4];
          for (uint32_t core = 0; core < CPUS; core++) {
            uint32_t phandle = 0x10 + core;
            plic[core * 4 + 0] = phandle;
            plic[core * 4 + 1] = 11;
            plic[core * 4 + 2] = phandle;
            plic[core * 4 + 3] = 9;
          }
          builder.add("interrupts-extended", plic, CPUS * 4);
          builder.add("phandle", 0x02);
        }
        builder.end();

        builder.begin("virtio@30000000");
        {
          uint64_t address = 0x30000000;
          uint32_t irq = 32;
          uint32_t regs[] = {CPU::hi32(address), CPU::lo32(address), 0x00,
                             0x1000};
          builder.add("compatible", "virtio,mmio");
          builder.add("reg", regs, 4);
          builder.add("interrupts", irq);
          builder.add("interrupt-parent", 0x02);
        }
        builder.end();
      }
      builder.end();
    }
    builder.end();

    builder.finish();

    return buffer;
  }

private:
  size_t size_;
  uint8_t *start_;
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

  new LinuxLauncher(128 * MB, receiver->linux(), receiver->initramfs(), 0);

  Device::destroy();

  return 0;
}
