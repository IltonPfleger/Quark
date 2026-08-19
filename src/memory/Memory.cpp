#include <BootInformation.hpp>
#include <architecture/CPU.hpp>
#include <memory/Memory.hpp>
#include <memory/operators.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

void Memory::init() {
  constexpr auto PageSize = Traits<Memory>::PageSize;
  constexpr auto RamStart = Traits<MemoryMap>::RamStart;
  constexpr auto RamEnd = Traits<MemoryMap>::RamEnd;

  TraceIn();

  new (&allocator_) Allocator();

  for (uintptr_t c = RamEnd - PageSize; c >= RamStart; c -= PageSize) {
    Chunk page(c, PageSize);
    if (page.overlaps(BootInformation::kernel()))
      continue;
    if (page.overlaps(__bmm))
      continue;
    if (page.overlaps(__emm))
      continue;
    if (page.overlaps(__pmm))
      continue;
    allocator_.insert(reinterpret_cast<void *>(page.start()), page.length());
  }

  TraceOut();
}

uintptr_t Memory::virt2phys(uintptr_t address) {
  if constexpr (Traits<Kernel>::Multitask) {
    return address - (Traits<MemoryMap>::RamStart - __amm.start());
  }
  return address;
}

uintptr_t Memory::phys2virt(uintptr_t address) {
  if constexpr (Traits<Kernel>::Multitask) {
    return address + (Traits<MemoryMap>::RamStart - __amm.start());
  }
  return address;
}

void *Memory::alloc(size_t size) {
  if (size == 0)
    return nullptr;

  CPU::IRQ::Guard _;

  spin_.acquire();

  TraceIn(size);

  void *chunk = allocator_.remove(size);

  assert(chunk, "Out of Memory!");

  TraceOut(chunk);

  spin_.release();

  return chunk;
}

void Memory::free(void *chunk, size_t size) {
  if (!chunk)
    return;

  CPU::IRQ::Guard _;

  spin_.acquire();

  TraceIn(chunk, size);

  allocator_.insert(chunk, size);

  TraceOut();

  spin_.release();
}

} // namespace QUARK
