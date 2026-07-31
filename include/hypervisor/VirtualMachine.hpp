#ifndef __QUARK_HYPERVISOR_VIRTUAL_MACHINE__
#define __QUARK_HYPERVISOR_VIRTUAL_MACHINE__

#include <Meta.hpp>
#include <hypervisor/VirtualMemoryManager.hpp>
#include <memory/Chunk.hpp>
#include <types.hpp>

namespace QUARK {

class VirtualMachine {
  public:
    VirtualMachine(const Chunk &&chunk)
        : memory_(static_cast<const Chunk &&>(chunk)) {}

    virtual void boot(uintmax_t, void *, void *) = 0;

    virtual bool read(uintptr_t target, uint32_t *destination) = 0;

    virtual bool write(uintptr_t target, uint32_t source) = 0;

    virtual void interrupt(uint32_t id) = 0;

    virtual const VirtualMemoryManager &memory() { return memory_; }

    virtual ~VirtualMachine() = default;

  private:
    VirtualMemoryManager memory_;
};

} // namespace QUARK

#endif
