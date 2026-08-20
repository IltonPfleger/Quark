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

constinit volatile size_t counter = 0;
constinit Semaphore created;

size_t size;
uint8_t buffer[BufferSize];

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

static void barrier() {
  static constinit volatile bool gsense = true;
  static constinit volatile int ready = Traits<QUARK::CPU>::Active;

  const bool sense = !__atomic_load_n(&gsense, __ATOMIC_ACQUIRE);
  const int position = __atomic_fetch_sub(&ready, 1, __ATOMIC_ACQ_REL);

  if (position == 1) {
    __atomic_store_n(&ready, Traits<QUARK::CPU>::Active, __ATOMIC_RELEASE);
    __atomic_store_n(&gsense, sense, __ATOMIC_RELEASE);
  } else {
    while (__atomic_load_n(&gsense, __ATOMIC_ACQUIRE) != sense)
      ;
  }
}

__attribute__((naked)) void *worker(void *) {
  static constexpr uintptr_t Address = Traits<MemoryMap>::Boot;

  CPU::IRQ::disable();

  size_t core = CPU::id();

  barrier();

  CPU::stack(0);

  if (core == Traits<CPU>::BSP) {
    for (size_t i = 0; i < size; i++) {
      reinterpret_cast<uint8_t *>(Address)[i] = buffer[i];
    }
  }

  barrier();

  CPU::mb();
  CPU::ib();

  reinterpret_cast<void (*)()>(Address)();

  return nullptr;
}

int main() {
  typedef Meta::GetFromTypeList<Traits<Ethernet>::Devices, 0>::Result Device;

  Device::init();

  auto *link = new LinkIPv4ToEthernet(Device::instance());
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
    if (i == CPU::id())
      continue;

    Thread::Criterion critetion(Thread::Criterion::NORMAL, i);
    new Thread(worker, nullptr, critetion);
  }

  worker(nullptr);
}
