#include <Thread.hpp>
#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <machine/Machine.hpp>
#include <network/link/LinkIPv4ToEthernet.hpp>
#include <network/protocols/TFTP.hpp>
#include <utility/Deferred.hpp>

using namespace QUARK;

static constexpr size_t KB = 1024;
static constexpr size_t MB = 1024 * KB;
static constexpr size_t BufferSize = 128 * MB;

constinit volatile size_t created = 0;
constinit volatile size_t counter = 0;

size_t size;
uint8_t buffer[BufferSize];

class Barrier {
public:
  __attribute__((always_inline)) static void
  wait(size_t count = Traits<CPU>::Active) {
    const size_t generation = __atomic_load_n(&generation_, __ATOMIC_ACQUIRE);

    if (__atomic_fetch_add(&arrived_, 1, __ATOMIC_ACQ_REL) + 1 == count) {
      __atomic_store_n(&arrived_, 0, __ATOMIC_RELEASE);
      __atomic_fetch_add(&generation_, 1, __ATOMIC_RELEASE);
      return;
    }

    while (__atomic_load_n(&generation_, __ATOMIC_ACQUIRE) == generation)
      ;
  }

private:
  static volatile inline size_t arrived_;
  static volatile inline size_t generation_;
};

void receive(TFTP &tftp) {
  IPv4::Address server(192, 168, 1, 100);
  int result = -1;
  int retries = 0;
  while (result < 0) {
    if (retries++ > 0) {
      Console::println("Failed To Retrieve The Image...");
    }
    result = tftp.request(server, "Q-U-A-R-K", buffer, BufferSize);
  }
  size = result;
}

__attribute__((naked)) void *worker(void *) {
  static constexpr uintptr_t Address = Traits<MemoryMap>::Boot;

  while (created != Traits<CPU>::Active)
    ;

  CPU::IRQ::disable();

  Barrier::wait();

  for (size_t i = 0; i < size; i++)
    reinterpret_cast<uint8_t *>(Address)[i] = buffer[i];

  Barrier::wait();

  reinterpret_cast<void (*)()>(Address)();

  return nullptr;
}

int main() {
  typedef Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result Device;

  Device::init();

  auto *link = new LinkIPv4ToEthernet(*Device::instance());
  auto *ipv4 = new IPv4(IPv4::Address(192, 168, 1, 101), *link);
  auto *udp = new UDP(*ipv4);
  auto *tftp = new TFTP(*udp);

  receive(*tftp);

  // delete link;
  // delete ipv4;
  // delete udp;
  // delete tftp;

  Device::destroy();

  for (size_t i = 0; i < Traits<CPU>::Active; i++) {
    Thread::Criterion critetion(Thread::Criterion::NORMAL, i);
    new Thread(worker, nullptr, critetion);
    created = created + 1;
  }
}
