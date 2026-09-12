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

#define ARTERY_PROJECT
#define NO_DATA_SOURCE

#include <boolean_filters.h>
#include <main_traits.h>
#include <seu.h>
#include <smartdata.h>
#include <transducer.h>
#include <transformer.h>

using DS = Dynamics_State;
using DS_Proxy = Interested_SmartData<DS::Unit::Wrap<DS::UNIT>>;
using OBRT_Fuser = Object_Recognition_And_Tracking_Fuser;
using OBRT_Fuser_Proxy = Interested_SmartData<
    OBRT_Fuser::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 10)>>;
using OBRT_Camera_Proxy = Interested_SmartData<
    OBRT_Fuser::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 11)>>;
using OBRT_LiDAR_Proxy = Interested_SmartData<
    OBRT_Fuser::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 12)>>;
using OBRT_RADAR_Proxy = Interested_SmartData<
    OBRT_Fuser::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 13)>>;

namespace QUARK {

static constexpr size_t MB = 1024 * 1024;

class Receiver {
public:
  Receiver(TFTP &tftp)
      : tftp_(tftp), buffer_(new uint8_t[BufferSize], BufferSize) {

    IPv4::Address server(192, 168, 1, 100);

    current_ = buffer_.data();
    remaining_ = buffer_.length();

    size_t size;

    size = tftp_.request(server, "RemoteBootVisionFive2Kernel", current_,
                         remaining_);
    new (&linux_) Span(current_, size);
    current_ += size;
    remaining_ -= size;

    size = tftp_.request(server, "RemoteBootVisionFive2InitRD.cpio", current_,
                         remaining_);
    new (&initramfs_) Span(current_, size);
    current_ += size;
    remaining_ -= size;

    size = tftp_.request(server, "RemoteBootVisionFive2EPOS", current_,
                         remaining_);
    new (&epos_) Span(current_, size);
    current_ += size;
    remaining_ -= size;
  }

  ~Receiver() { delete[] buffer_.data(); }

  const auto &linux() const { return linux_; }
  const auto &initramfs() const { return initramfs_; }
  const auto &epos() const { return epos_; }

private:
  static constexpr size_t BufferSize = 128 * MB;

private:
  TFTP &tftp_;
  Span<uint8_t> buffer_;
  uint8_t *current_;
  size_t remaining_;
  Span<const uint8_t> linux_;
  Span<const uint8_t> initramfs_;
  Span<const uint8_t> epos_;
};

class LinuxLauncher {
public:
  static constexpr uint32_t CPUS = 1;

  using SerialDevice = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
  using Serial = virtio::Console<SerialDevice, 0x30000000, 32>;
  using InterruptController = VirtualPLIC<CPUS, 0xc000000>;

  using NetworkDevice =
      Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result;
  using Network = virtio::Network<NetworkDevice, 0x30200000, 50>;

  using LinuxMachine =
      GenericVirtualMachine<CPUS, Serial, Network, InterruptController>;

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

    builder.begin("");
    {
      builder.add("#address-cells", 2);
      builder.add("#size-cells", 2);
      builder.add("compatible", "riscv-virtio");
      builder.add("model", "riscv-virtio,qemu");

      builder.begin("chosen");
      {
        builder.add("bootargs", "console=hvc0 loglevel=8");

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
        uint64_t start = reinterpret_cast<uint64_t>(start_);
        uint32_t regs[] = {CPU::hi32(start), CPU::lo32(start), CPU::hi32(size_),
                           CPU::lo32(size_)};
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

          uint32_t plic[CPUS * 2];
          for (uint32_t core = 0; core < CPUS; core++) {
            uint32_t phandle = 0x10 + core;
            plic[core * 2] = phandle;
            plic[core * 2 + 1] = 9;
          }
          builder.add("interrupts-extended", plic, CPUS * 2);
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

        builder.begin("virtio@30200000");
        {
          uint64_t address = 0x30200000;
          uint32_t irq = 50;
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

class EPOS_Launcher {
  using NetworkDevice =
      Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result;
  using Network = virtio::Network<VirtualSwitch<NetworkDevice>, 0x30200000, 50>;
  using SerialDevice = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;
  using Serial = virtio::Console<SerialDevice, 0x30000000, 32>;
  using InterruptController = VirtualPLIC<1, 0xc000000>;
  using EPOS_Machine =
      GenericVirtualMachine<1, InterruptController, Serial, Network>;

public:
  EPOS_Launcher(size_t size, const Span<const uint8_t> &epos, size_t core)
      : buffer_(static_cast<uint8_t *>(Memory::alloc(size)), size),
        machine_(buffer_.data(), buffer_.length(), core) {
    memset(buffer_.data(), 0, buffer_.length());
    memcpy(buffer_.data(), epos.data(), epos.length());
    machine_.boot(0, buffer_.data(), (void *)1ULL);
  }

private:
  Span<uint8_t> buffer_;
  EPOS_Machine machine_;
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
//         new Thread(worker, this, Thread::Criterion{Thread::Criterion::NORMAL,
//         3});
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

void smartdata() {
  TSTP::init();

  static constexpr int EXPIRY = 150'000;

  SEU_SmartData *seu = new SEU_SmartData();

  Unit_Dev_Expiry::List *ud_list;
  ud_list = new Unit_Dev_Expiry::List();
  auto *mu =
      new MU_Arrival_Dep(ud_list, Dynamics_State::UNIT, 16, 100000, 100000);
  seu->add_boolean_filter(mu);

  // MONITOR
  ud_list = new Unit_Dev_Expiry::List();
  ud_list->insert(
      (new Unit_Dev_Expiry(Dynamics_State::UNIT, 16, EXPIRY))->link());
  ud_list->insert(
      (new Unit_Dev_Expiry(OBRT_LiDAR_Proxy::UNIT, 21, EXPIRY))->link());
  ud_list->insert(
      (new Unit_Dev_Expiry(OBRT_Camera_Proxy::UNIT, 20, EXPIRY))->link());
  ud_list->insert(
      (new Unit_Dev_Expiry(OBRT_Fuser_Proxy::UNIT, 23, EXPIRY))->link());
  auto *monitor = new Monitoring(ud_list);
  seu->add_boolean_filter(monitor);

  // CAMERA
  ud_list = new Unit_Dev_Expiry::List();
  ud_list->insert(
      (new Unit_Dev_Expiry(Dynamics_State::UNIT, 16, EXPIRY))->link());
  auto *obrtc =
      new MU_Arrival_Dep(ud_list, OBRT_Camera_Proxy::UNIT, 20, EXPIRY, 100000);
  seu->add_boolean_filter(obrtc);

  // LIDAR
  ud_list = new Unit_Dev_Expiry::List();
  ud_list->insert(
      (new Unit_Dev_Expiry(Dynamics_State::UNIT, 16, EXPIRY))->link());
  auto *obrtl =
      new MU_Arrival_Dep(ud_list, OBRT_LiDAR_Proxy::UNIT, 21, EXPIRY, 100000);
  seu->add_boolean_filter(obrtl);

  // FUSER
  ud_list = new Unit_Dev_Expiry::List();
  ud_list->insert(
      (new Unit_Dev_Expiry(OBRT_Camera_Proxy::UNIT, 20, EXPIRY))->link());
  ud_list->insert(
      (new Unit_Dev_Expiry(OBRT_LiDAR_Proxy::UNIT, 21, EXPIRY))->link());
  auto *obrtf = new MU_Arrival_Dep(
      ud_list, Object_Recognition_And_Tracking_Fuser::UNIT, 23, EXPIRY, 100000);
  seu->add_boolean_filter(obrtf);

  // RSS
  Road_Parameters *rp = new Road_Parameters(0, 0, 0, 0, 0);
  rp->set_default();
  ud_list = new Unit_Dev_Expiry::List();
  ud_list->insert(
      (new Unit_Dev_Expiry(Dynamics_State::UNIT, 16, EXPIRY))->link());
  ud_list->insert((new Unit_Dev_Expiry(OBRT_Fuser::UNIT, 23, EXPIRY))->link());
  RSS_Safe_Distance *rss = new RSS_Safe_Distance(ud_list, rp, rp, EXPIRY);
  seu->add_boolean_filter(rss);

  QUARK::Delay(QUARK::Microsecond(1'000));

  new DS_Proxy(DS_Proxy::Region(0, 0, 0, 100, DS_Proxy::now(), INFINITE),
               EXPIRY, 5'000, SmartData::SINGLE, SmartData::ANY, 16);

  new OBRT_Fuser_Proxy(
      OBRT_Fuser_Proxy::Region(0, 0, 0, 100, OBRT_Fuser_Proxy::now(), INFINITE),
      EXPIRY, 40'000, SmartData::SINGLE, SmartData::ANY, 23);

  new OBRT_Camera_Proxy(OBRT_Camera_Proxy::Region(
                            0, 0, 0, 100, OBRT_Camera_Proxy::now(), INFINITE),
                        EXPIRY, 150'000, SmartData::SINGLE, SmartData::ANY, 20);

  new OBRT_LiDAR_Proxy(
      OBRT_LiDAR_Proxy::Region(0, 0, 0, 100, OBRT_LiDAR_Proxy::now(), INFINITE),
      EXPIRY, 100'000, SmartData::SINGLE, SmartData::ANY, 21);

  new OBRT_RADAR_Proxy(
      OBRT_RADAR_Proxy::Region(0, 0, 0, 100, OBRT_RADAR_Proxy::now(), INFINITE),
      EXPIRY, 40'000, SmartData::SINGLE, SmartData::ANY, 22);
}

int main() {
  using namespace QUARK;

  typedef Meta::GetFromTypeList<QUARK::Traits<QUARK::Ethernet>::Devices,
                                0>::Result Device;

  Device::init();

  auto *link = new QUARK::LinkIPv4ToEthernet(Device::instance());
  auto *ipv4 = new QUARK::IPv4(IPv4::Address(192, 168, 1, 101), *link);
  auto *udp = new QUARK::UDP(*ipv4);
  auto *tftp = new QUARK::TFTP(*udp);
  auto *receiver = new Receiver(*tftp);

  delete tftp;
  delete udp;
  delete ipv4;
  delete link;

  new LinuxLauncher(256 * MB, receiver->linux(), receiver->initramfs(), 3);

  // DYNAMICS STATE
  // new EPOS_Launcher(MemorySize / 2, receiver->epos(), 1);
  // while (QUARK::sbi::Counter::counter_ != 1)
  //  ;

  //// Fuser
  // new EPOS_Launcher(MemorySize / 2, receiver->epos(), 1);
  // while (QUARK::sbi::Counter::counter_ != 2)
  //   ;

  //// RADAR
  // new EPOS_Launcher(MemorySize / 2, receiver->epos(), 2);
  // while (QUARK::sbi::Counter::counter_ != 3)
  //   ;

  //// LiDAR
  // new EPOS_Launcher(MemorySize / 2, receiver->epos(), 2);
  // while (QUARK::sbi::Counter::counter_ != 4)
  //   ;

  //// Camera
  // new EPOS_Launcher(MemorySize / 2, receiver->epos(), 1);
  // while (QUARK::sbi::Counter::counter_ != 5)
  //   ;

  // QUARK::Delay(QUARK::Microsecond(5'000'000));

  // smartdata();

  //// new NetworkVampire<VirtualSwitch<Device>>();

  // while (1) {
  //     QUARK::Delay(QUARK::Microsecond(100'000'000));
  // }

  // for (int i = 0; i < 10; i++) {
  //     new Responsive_SmartData<Overhead>(i, 5'000, SmartData::ADVERTISED);
  // }

  return 0;
}
