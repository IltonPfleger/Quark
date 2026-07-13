// #include <SDs.hpp>
#include <NetworkVampire.hpp>
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

// #define ARTERY_PROJECT
// #define NO_DATA_SOURCE
//
// #include <boolean_filters.h>
// #include <main_traits.h>
// #include <seu.h>
// #include <smartdata.h>
// #include <transducer.h>
// #include <transformer.h>
//
// using DS                = Dynamics_State;
// using DS_Proxy          = Interested_SmartData<DS::Unit::Wrap<DS::UNIT>>;
// using OBRT_Fuser        = Object_Recognition_And_Tracking_Fuser;
// using OBRT_Fuser_Proxy  = Interested_SmartData<OBRT_Fuser::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 10)>>;
// using OBRT_Camera_Proxy = Interested_SmartData<OBRT_Fuser::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 11)>>;
// using OBRT_LiDAR_Proxy  = Interested_SmartData<OBRT_Fuser::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 12)>>;
// using OBRT_RADAR_Proxy  = Interested_SmartData<OBRT_Fuser::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 13)>>;

namespace QUARK {

class Receiver {
  public:
    Receiver(TFTP &tftp)
        : tftp_(tftp),
          buffer_(new uint8_t[BufferSize], BufferSize) {

        IPv4::Address server(192, 168, 1, 100);

        current_   = buffer_.data();
        remaining_ = buffer_.length();

        size_t size;

        size = tftp_.request(server, "RemoteBootVisionFive2Kernel", current_, remaining_);
        new (&linux_) Span(current_, size);
        current_ += size;
        remaining_ -= size;

        size = tftp_.request(server, "RemoteBootVisionFive2InitRD.cpio", current_, remaining_);
        new (&initramfs_) Span(current_, size);
        current_ += size;
        remaining_ -= size;

        size = tftp_.request(server, "RemoteBootVisionFive2EPOS", current_, remaining_);
        new (&epos_) Span(current_, size);
        current_ += size;
        remaining_ -= size;
    }

    ~Receiver() { delete[] buffer_.data(); }

    const auto &linux() const { return linux_; }
    const auto &initramfs() const { return initramfs_; }
    const auto &epos() const { return epos_; }

  private:
    static constexpr size_t BufferSize = 128 * 1024 * 1024;

  private:
    TFTP &tftp_;
    Span<uint8_t> buffer_;
    unsigned char *current_;
    size_t remaining_;
    Span<const uint8_t> linux_;
    Span<const uint8_t> initramfs_;
    Span<const uint8_t> epos_;
};

class LinuxLauncher {
  public:
    using SerialDevice        = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
    using Serial              = virtio::Console<SerialDevice, 0x30000000, 32>;
    using InterruptController = VirtualPLIC<0xc000000>;
    using LinuxMachine        = GenericVirtualMachine<InterruptController, Serial>;

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
    static constexpr size_t MB = 1024 * 1024;

  private:
    size_t size_;
    unsigned char *start_;
    Span<const uint8_t> initramfs_;
    unsigned char *dtb_;
    LinuxMachine *vm_;
};

class EPOS_Launcher {
    using NetworkDevice       = Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result;
    using Network             = virtio::Network<VirtualSwitch<NetworkDevice>, 0x30200000, 50>;
    using SerialDevice        = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
    using Serial              = virtio::Console<SerialDevice, 0x30000000, 32>;
    using InterruptController = VirtualPLIC<0xc000000>;
    using EPOS_Machine        = GenericVirtualMachine<InterruptController, Serial, Network>;

  public:
    EPOS_Launcher(size_t size, const Span<const uint8_t> &epos, Thread::Criterion criterion)
        : epos_(epos),
          buffer_(static_cast<unsigned char *>(Memory::alloc(size)), size),
          machine_(buffer_.data(), buffer_.length()),
          thread_(worker, this, criterion) {}

  private:
    static void *worker(void *pointer) {
        TraceIn(Thread::running());
        auto *self = reinterpret_cast<EPOS_Launcher *>(pointer);
        memset(self->buffer_.data(), 0, self->buffer_.length());
        memcpy(self->buffer_.data(), self->epos_.data(), self->epos_.length());
        self->machine_.boot(1);
        return nullptr;
    }

  private:
    const Span<const uint8_t> &epos_;
    Span<uint8_t> buffer_;
    EPOS_Machine machine_;
    Thread thread_;
};

} // namespace QUARK

// class Overhead : public Transducer<SmartData::Unit::Antigravity> {
//     friend Responsive_SmartData<Overhead>;
//
//   public:
//     static const bool active             = true;
//     static const Uncertainty UNCERTAINTY = UNKNOWN;
//     static const Type TYPE               = SENSOR | ACTUATOR;
//
//   public:
//     Overhead(const Device_Id &dev)
//         : _value(0) {
//         new Thread(worker, this, Thread::Criterion{Thread::Criterion::NORMAL, 3});
//     }
//
//     ~Overhead() {}
//
//     static void *worker(void *pointer) {
//         Overhead *self = reinterpret_cast<Overhead *>(pointer);
//         while (1) {
//             self->notify();
//             QUARK::Delay(5'000);
//         }
//         return nullptr;
//     }
//
//     virtual Value sense() { return _value; }
//     virtual Signature signature() { return 0; }
//     virtual void actuate(const Value &value) { _value = value; }
//
//   private:
//     Value _value;
// };
//
// void smartdata() {
//     TSTP::init();
//
//     // SEU_SmartData *seu = new SEU_SmartData();
//
//     // Road_Parameters *rp = new Road_Parameters(0, 0, 0, 0, 0);
//     // rp->set_default();
//
//     // Unit_Dev_Expiry::List *ud_list;
//
//     // ud_list = new Unit_Dev_Expiry::List();
//     // ud_list->insert((new Unit_Dev_Expiry(Dynamics_State::UNIT, 16, 100000))->link());
//     // seu->add_boolean_filter(new MU_Arrival_Dep(ud_list, OBRT_Camera_Proxy::UNIT, 20, 100000, 100000));
//
//     // ud_list = new Unit_Dev_Expiry::List();
//     // seu->add_boolean_filter(new MU_Arrival_Dep(ud_list, OBRT_LiDAR_Proxy::UNIT, 21, 100000, 100000));
//
//     // ud_list = new Unit_Dev_Expiry::List();
//     // seu->add_boolean_filter(new MU_Arrival_Dep(ud_list, OBRT_RADAR_Proxy::UNIT, 22, 100000, 100000));
//
//     // ud_list = new Unit_Dev_Expiry::List();
//     // seu->add_boolean_filter(new MU_Arrival_Dep(ud_list, OBRT_Fuser_Proxy::UNIT, 23, 100000, 100000));
//
//     // ud_list = new Unit_Dev_Expiry::List();
//     // ud_list->insert((new Unit_Dev_Expiry(Dynamics_State::UNIT, 16, 100000))->link());
//     // ud_list->insert((new Unit_Dev_Expiry(OBRT_Fuser::UNIT, 23, 100000))->link());
//     // RSS_Safe_Distance *rss = new RSS_Safe_Distance(ud_list, rp, rp, 100000);
//     // seu->add_boolean_filter(rss);
//
//     new DS_Proxy(DS_Proxy::Region(0, 0, 0, 100, DS_Proxy::now(), INFINITE), 5'000);
//     new OBRT_Fuser_Proxy(OBRT_Fuser_Proxy::Region(0, 0, 0, 100, OBRT_Fuser_Proxy::now(), INFINITE), 40'000);
//     new OBRT_Camera_Proxy(OBRT_Camera_Proxy::Region(0, 0, 0, 100, OBRT_Camera_Proxy::now(), INFINITE), 150'000);
//     new OBRT_LiDAR_Proxy(OBRT_LiDAR_Proxy::Region(0, 0, 0, 100, OBRT_LiDAR_Proxy::now(), INFINITE), 100'000);
//     new OBRT_RADAR_Proxy(OBRT_RADAR_Proxy::Region(0, 0, 0, 100, OBRT_RADAR_Proxy::now(), INFINITE), 40'000);
//     new Antigravity_Proxy(Antigravity_Proxy::Region(0, 0, 0, 100, Antigravity_Proxy::now(), INFINITE), 10'000);
// }

int main() {
    using namespace QUARK;

    typedef QUARK::Meta::GetFromTypeList<QUARK::Traits<QUARK::Ethernet>::Devices, 0>::Result Device;

    auto *link     = new QUARK::LinkIPv4ToEthernet(*Device::instance());
    auto *ipv4     = new QUARK::IPv4(IPv4::Address(192, 168, 1, 101), *link);
    auto *udp      = new QUARK::UDP(*ipv4);
    auto *tftp     = new QUARK::TFTP(*udp);
    auto *receiver = new Receiver(*tftp);

    const size_t MemorySize = 1024 * 1024 * 128;
    // const auto &linux       = receiver->linux();
    // const auto &initramfs   = receiver->initramfs();
    const auto &epos = receiver->epos();

    // new LinuxLauncher(MemorySize, linux, initramfs, QUARK::Thread::Criterion(QUARK::Thread::Criterion::NORMAL, 3));

    //// DYNAMICS STATE
    // new EPOS_Launcher(MemorySize / 2, epos, QUARK::Thread::Criterion(QUARK::Thread::Criterion::NORMAL, 1));
    // while (QUARK::sbi::Counter::counter_ != 1)
    //     ;

    //// Fuser
    // new EPOS_Launcher(MemorySize / 2, epos, QUARK::Thread::Criterion(QUARK::Thread::Criterion::NORMAL, 1));
    // while (QUARK::sbi::Counter::counter_ != 2)
    //     ;

    //// RADAR
    // new EPOS_Launcher(MemorySize / 2, epos, QUARK::Thread::Criterion(QUARK::Thread::Criterion::NORMAL, 2));
    // while (QUARK::sbi::Counter::counter_ != 3)
    //     ;

    //// LiDAR
    // new EPOS_Launcher(MemorySize / 2, epos, QUARK::Thread::Criterion(QUARK::Thread::Criterion::NORMAL, 2));
    // while (QUARK::sbi::Counter::counter_ != 4)
    //     ;

    // Camera
    new EPOS_Launcher(MemorySize / 2, epos, QUARK::Thread::Criterion(QUARK::Thread::Criterion::NORMAL, 1));
    while (QUARK::sbi::Counter::counter_ != 5)
        ;

    // QUARK::Delay(QUARK::Microsecond(5'000'000));

    // smartdata();

    // new NetworkVampire<VirtualSwitch<Device>>();

    // for (int i = 0; i < 10; i++) {
    //     new Responsive_SmartData<Overhead>(i, 5'000, SmartData::ADVERTISED);
    // }

    return 0;
}
