#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <machine/Machine.hpp>
#include <network/link/LinkIPv4ToEthernet.hpp>
#include <network/protocols/TFTP.hpp>

using namespace QUARK;

static constexpr size_t k_kb             = 1024;
static constexpr size_t k_mb             = 1024 * k_kb;
static constexpr size_t k_size           = 128 * k_mb;
static volatile constinit size_t g_sense = 0;
static size_t g_size;
static uint8_t g_buffer[k_size];

class Receiver {
  public:
    Receiver(TFTP &tftp) {
        IPv4::Address server(192, 168, 1, 100);
        g_size = tftp.request(server, "QUARK-OS-Remote-Boot-Image", g_buffer, k_size);
    }
};

static void barrier() {
    static constinit volatile bool gsense = 1;
    static constinit volatile int ready   = Traits<QUARK::CPU>::Active;

    auto sense   = !CPU::Atomic::load(gsense);
    int position = CPU::Atomic::fdec(ready);

    if (position == 1) {
        CPU::Atomic::store(ready, Traits<QUARK::CPU>::Active);
        CPU::Atomic::store(gsense, sense);
    } else {
        while (CPU::Atomic::load(gsense) != sense)
            ;
    }
}

static void *worker(void *) {
    while (g_sense != 1)
        ;

    CPU::IRQ::disable();

    barrier();

    int core = CPU::id();

    barrier();

    if (core == Traits<CPU>::BSP) {
        for (size_t i = 0; i < g_size; i++) {
            reinterpret_cast<uint8_t *>(Traits<MemoryMap>::BootStart)[i] = g_buffer[i];
        }
    }

    barrier();

    return reinterpret_cast<void *(*)()>(Traits<MemoryMap>::BootStart)();
}

int main() {
    typedef Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result Device;

    auto *link = new QUARK::LinkIPv4ToEthernet(*Device::instance());
    auto *ipv4 = new QUARK::IPv4(IPv4::Address(192, 168, 1, 101), *link);
    auto *udp  = new QUARK::UDP(*ipv4);
    auto *tftp = new QUARK::TFTP(*udp);

    Receiver receiver(*tftp);

    for (size_t i = 0; i < Traits<CPU>::Active; i++) {
        new Thread(worker, (void *)i, Thread::Criterion(Thread::Criterion::NORMAL, i));
    }

    g_sense = 1;

    while (1)
        ;
}
