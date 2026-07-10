#include <Thread.hpp>
#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <machine/Machine.hpp>
#include <network/link/LinkIPv4ToEthernet.hpp>
#include <network/protocols/TFTP.hpp>

using namespace QUARK;

static constexpr size_t k_kb               = 1024;
static constexpr size_t k_mb               = 1024 * k_kb;
static constexpr size_t k_size             = 128 * k_mb;
static volatile constinit size_t g_counter = 0;
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
    static constinit volatile bool gsense = true;
    static constinit volatile int ready   = Traits<QUARK::CPU>::Active;

    bool sense   = !__atomic_load_n(&gsense, __ATOMIC_ACQUIRE);
    int position = __atomic_fetch_sub(&ready, 1, __ATOMIC_ACQ_REL);

    if (position == 1) {
        __atomic_store_n(&ready, Traits<QUARK::CPU>::Active, __ATOMIC_RELEASE);
        __atomic_store_n(&gsense, sense, __ATOMIC_RELEASE);
    } else {
        while (__atomic_load_n(&gsense, __ATOMIC_ACQUIRE) != sense) {
        }
    }
}

__attribute__((naked)) static void jumper() {
    barrier();
    for (size_t i = 0; i < g_size; i++)
        reinterpret_cast<uint8_t *>(Traits<MemoryMap>::BootStart)[i] = g_buffer[i];
    barrier();
    reinterpret_cast<void (*)()>(Traits<MemoryMap>::BootStart)();
}

static void *worker(void *) {
    CPU::IRQ::disable();

    CPU::Atomic::finc(g_counter);

    while (g_counter != Traits<CPU>::Active)
        Thread::yield();

    jumper();
    return nullptr;
}

int main() {
    typedef Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result Device;

    auto *link = new QUARK::LinkIPv4ToEthernet(*Device::instance());
    auto *ipv4 = new QUARK::IPv4(IPv4::Address(192, 168, 1, 101), *link);
    auto *udp  = new QUARK::UDP(*ipv4);
    auto *tftp = new QUARK::TFTP(*udp);

    Receiver receiver(*tftp);

    for (size_t i = 0; i < Traits<CPU>::Active; i++) {
        new Thread(worker, nullptr, Thread::Criterion(Thread::Criterion::NORMAL, i));
    }
}
