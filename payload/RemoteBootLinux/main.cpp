#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <architecture/VirtualCPU.hpp>
#include <architecture/VirtualPLIC.hpp>
#include <hypervisor/GenericVirtualMachine.hpp>
#include <hypervisor/VirtualSwitch.hpp>
#include <hypervisor/dtb/FDT_Builder.hpp>
#include <hypervisor/virtio/Console.hpp>
#include <hypervisor/virtio/Network.hpp>
#include <libraries/libc/string.h>
#include <machine/Machine.hpp>
#include <network/link/LinkIPv4ToEthernet.hpp>
#include <network/protocols/TFTP.hpp>
#include <utility/Debug.hpp>

using namespace QUARK;

class Receiver {
  public:
    Receiver(TFTP &tftp)
        : tftp_(tftp),
          buffer_(new uint8_t[BufferSize], BufferSize) {

        IPv4::Address server(192, 168, 1, 100);

        current_   = buffer_.data();
        remaining_ = buffer_.length();

        int size;

        size = tftp_.request(server, "RemoteBootVisionFive2Kernel", current_, remaining_);
        assert(size > 0);
        new (&linux_) Span(current_, size);
        current_ += size;
        remaining_ -= size;

        size = tftp_.request(server, "RemoteBootVisionFive2InitRD.cpio", current_, remaining_);
        assert(size > 0);
        new (&initramfs_) Span(current_, size);
        current_ += size;
        remaining_ -= size;
    }

    ~Receiver() { delete[] buffer_.data(); }

    const auto &linux() const { return linux_; }
    const auto &initramfs() const { return initramfs_; }

  private:
    static constexpr size_t BufferSize = 128 * 1024 * 1024;

  private:
    TFTP &tftp_;
    Span<uint8_t> buffer_;
    unsigned char *current_;
    size_t remaining_;
    Span<const uint8_t> linux_;
    Span<const uint8_t> initramfs_;
};

class LinuxLauncher {
  public:
    using SerialDevice        = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
    using NetworkDevice       = Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result;
    using Serial              = virtio::Console<SerialDevice, 0x30000000, 32>;
    using Network             = virtio::Network<NetworkDevice, 0x40000000, 50>;
    using InterruptController = VirtualPLIC<0xc000000>;
    using LinuxMachine        = GenericVirtualMachine<InterruptController, Serial, Network>;

    LinuxLauncher(size_t size, Span<const uint8_t> kernel, Span<const uint8_t> initramfs, Thread::Criterion criterion)
        : size_(size),
          start_(nullptr),
          initramfs_(initramfs),
          dtb_(nullptr),
          vm_(nullptr) {
        start_ = static_cast<unsigned char *>(Memory::alloc(size_));

        uint8_t *current = align(start_, 4 * MB);
        memcpy(current, kernel, kernel.length());
        current += kernel.length();

        current = align(current, 4 * MB);
        memcpy(current, initramfs, initramfs.length());
        initramfs_ = Span<const uint8_t>(current, initramfs.length());
        current += initramfs.length();

        current = align(current, 4 * MB);
        dtb(current, size_ - initramfs.length() - kernel.length());

        new Thread(worker, this, criterion);
    }

    static void *worker(void *pointer) {
        LinuxLauncher *self = reinterpret_cast<LinuxLauncher *>(pointer);
        Console::println("\n *** Linux is at core ", CPU::id(), " ***");
        LinuxMachine *vm = new LinuxMachine(self->start_, self->size_);
        vm->boot(0, self->dtb_);
        return nullptr;
    }

    static unsigned char *align(unsigned char *pointer, long alignment) {
        uintptr_t address = reinterpret_cast<long>(pointer);
        address           = (address + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<unsigned char *>(address);
    }

    size_t dtb(void *buffer, size_t capacity) {
        FDT_Builder fdt(buffer, capacity);

        dtb_          = static_cast<unsigned char *>(buffer);
        uint64_t base = reinterpret_cast<uint64_t>(start_);

        fdt.begin("");
        {
            fdt.add("#address-cells", 2);
            fdt.add("#size-cells", 2);
            fdt.add("compatible", "riscv-virtio");
            fdt.add("model", "riscv-virtio,qemu");

            fdt.begin("chosen");
            {
                uint64_t start   = reinterpret_cast<uint64_t>(initramfs_.data());
                uint64_t end     = start + initramfs_.length();
                uint32_t regs0[] = {CPU::hi32(start), CPU::lo32(start)};
                uint32_t regs1[] = {CPU::hi32(end), CPU::lo32(end)};
                fdt.add("bootargs", "console=hvc0 loglevel=8");
                fdt.add("linux,initrd-start", regs0, 2);
                fdt.add("linux,initrd-end", regs1, 2);
            }
            fdt.end();

            fdt.begin("cpus");
            {
                fdt.add("#address-cells", 1);
                fdt.add("#size-cells", 0u);
                fdt.add("timebase-frequency", 0x989680);
                fdt.begin("cpu@0");
                {
                    fdt.add("device_type", "cpu");
                    fdt.add("reg", 0u);
                    fdt.add("status", "okay");
                    fdt.add("compatible", "riscv");
                    fdt.add("riscv,isa", "rv64imafdcsu");
                    fdt.add("mmu-type", "riscv,sv39");
                    fdt.begin("interrupt-controller");
                    {
                        fdt.add("#interrupt-cells", 1);
                        fdt.add("interrupt-controller");
                        fdt.add("compatible", "riscv,cpu-intc");
                        fdt.add("phandle", 0x01);
                    }
                    fdt.end();
                }
                fdt.end();
            }
            fdt.end();

            fdt.begin("memory");
            {
                fdt.add("device_type", "memory");
                uint32_t regs[] = {CPU::hi32(base), CPU::lo32(base), CPU::hi32(size_), CPU::lo32(size_)};
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
                    fdt.add("compatible", "riscv,plic0");

                    uint32_t regs0[] = {0x00, 0xc000000, 0x00, 0x4000000};
                    fdt.add("reg", regs0, 4);

                    fdt.add("interrupt-controller");
                    fdt.add("#interrupt-cells", 1);
                    fdt.add("riscv,ndev", 0x35);

                    uint32_t regs1[] = {0x01, 0x0b, 0x01, 0x09};
                    fdt.add("interrupts-extended", regs1, 4);
                    fdt.add("phandle", 0x02);
                }
                fdt.end();

                fdt.begin("virtio_mmio@30000000");
                {
                    uint64_t address = 0x30000000;
                    uint32_t irq     = 32;
                    uint32_t regs[]  = {CPU::hi32(address), CPU::lo32(address), 0x00, 0x1000};
                    fdt.add("compatible", "virtio,mmio");
                    fdt.add("reg", regs, 4);
                    fdt.add("interrupts", irq);
                    fdt.add("interrupt-parent", 0x02);
                }
                fdt.end();
                fdt.begin("virtio_mmio@40000000");
                {
                    uint64_t address = 0x40000000;
                    uint32_t irq     = 50;
                    uint32_t regs[]  = {CPU::hi32(address), CPU::lo32(address), 0x00, 0x1000};
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
    static constexpr size_t MB = 1024 * 1024;

  private:
    size_t size_;
    unsigned char *start_;
    Span<const uint8_t> initramfs_;
    unsigned char *dtb_;
    LinuxMachine *vm_;
};

int main() {
    using namespace QUARK;

    typedef Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result Device;

    auto *link = new QUARK::LinkIPv4ToEthernet(*Device::instance());
    auto *ipv4 = new QUARK::IPv4(IPv4::Address(192, 168, 1, 101), *link);
    auto *udp  = new QUARK::UDP(*ipv4);
    auto *tftp = new QUARK::TFTP(*udp);

    Receiver receiver(*tftp);

    const size_t MemorySize = 1024 * 1024 * 128;

    const auto &linux     = receiver.linux();
    const auto &initramfs = receiver.initramfs();

    new LinuxLauncher(MemorySize, linux, initramfs, QUARK::Thread::Criterion(QUARK::Thread::Criterion::NORMAL, 3));
}
