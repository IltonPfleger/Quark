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
    using SerialDevice        = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
    using Serial              = virtio::Console<SerialDevice, 0x30000000, 32>;
    using InterruptController = VirtualPLIC<0xc000000>;
    using LinuxMachine        = GenericVirtualMachine<Serial, InterruptController>;

    LinuxLauncher(size_t size, Span<const uint8_t> kernel, Span<const uint8_t> initramfs)
        : size_(size),
          start_(nullptr),
          initramfs_(initramfs),
          dtb_(nullptr),
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

        new Thread(worker, this);
    }

    static void *worker(void *pointer) {
        LinuxLauncher *self = reinterpret_cast<LinuxLauncher *>(pointer);
        Console::println("\n *** Linux is at core ", CPU::id(), " ***");
        auto *vm = new LinuxMachine(self->start_, self->size_);
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

        dtb_                  = static_cast<unsigned char *>(buffer);
        uint64_t base         = reinterpret_cast<uint64_t>(start_);
        uint64_t initrd_start = reinterpret_cast<uint64_t>(initramfs_.data());
        uint64_t initrd_end   = initrd_start + initramfs_.length();

        fdt.begin("");
        {
            fdt.add("#address-cells", 2);
            fdt.add("#size-cells", 2);
            fdt.add("compatible", "riscv-virtio");
            fdt.add("model", "riscv-virtio,qemu");

            fdt.begin("chosen");
            {
                fdt.add("bootargs", "console=hvc0 loglevel=8");
                uint32_t regs0[] = {CPU::hi32(initrd_start), CPU::lo32(initrd_start)};
                fdt.add("linux,initrd-start", regs0, 2);
                uint32_t regs1[] = {CPU::hi32(initrd_end), CPU::lo32(initrd_end)};
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

    LinuxLauncher vm0(128 * 1024 * 1024, kernel, initramfs);
    LinuxLauncher vm1(128 * 1024 * 1024, kernel, initramfs);
    LinuxLauncher vm2(128 * 1024 * 1024, kernel, initramfs);

    while (1)
        Delay(Second(1));

    return 0;
}
