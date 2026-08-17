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
uint8_t stack[1024];

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

void *worker(void *) {
  static constexpr uintptr_t Address = Traits<MemoryMap>::Boot;

  while (created != Traits<CPU>::Active)
    ;

  CPU::IRQ::disable();

  size_t local = __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);

  while (counter != Traits<CPU>::Active)
    ;

  if (local == Traits<CPU>::Active - 1) {
    for (size_t i = 0; i < size; i++)
      reinterpret_cast<uint8_t *>(Address)[i] = buffer[i];
  }

  __atomic_fetch_sub(&created, 1, __ATOMIC_SEQ_CST);

  while (created != 0)
    ;

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
