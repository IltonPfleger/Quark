#ifndef __QUARK_HYPERVISOR_VIRTUAL_MACHINE__
#define __QUARK_HYPERVISOR_VIRTUAL_MACHINE__

#include <Meta.hpp>
#include <architecture/VirtualCPU.hpp>
#include <memory/Chunk.hpp>
#include <types.hpp>

namespace QUARK {

class VirtualMachine {
public:
  explicit VirtualMachine(Chunk chunk) : memory_(Meta::Move(chunk)) {}
  virtual ~VirtualMachine() = default;

  virtual void boot(uintmax_t, void *, void *) = 0;

  virtual bool read(uintptr_t target, uint32_t *destination) = 0;

  virtual bool write(uintptr_t target, uint32_t source) = 0;

  virtual void interrupt(uint32_t id) = 0;

  virtual VirtualCPU &cpu(size_t) = 0;

  virtual const Chunk &memory() const { return memory_; }

private:
  const Chunk memory_;
};

} // namespace QUARK

#endif
