#ifndef __QUARK_HYPERVISOR_GENERIC_VIRTUAL_MACHINE__
#define __QUARK_HYPERVISOR_GENERIC_VIRTUAL_MACHINE__

#include <Meta.hpp>
#include <Thread.hpp>
#include <architecture/VirtualCPU.hpp>
#include <hypervisor/VirtualInterruptController.hpp>
#include <hypervisor/VirtualMachine.hpp>
#include <synchronization/Semaphore.hpp>
#include <types.hpp>

namespace QUARK {

template <size_t CORES, typename... DEVICES>
class GenericVirtualMachine : public VirtualMachine {
public:
  GenericVirtualMachine(void *entry, size_t size, size_t offset)
      : VirtualMachine(Chunk(entry, size)),
        cpus_(create(Meta::MakeIndexSequence<CORES>{}, offset)),
        devices_(*this) {}

  ~GenericVirtualMachine() = default;

  //~GenericVirtualMachine() {
  //  for (int i = 0; i < CORES; i++) {
  //  }
  //  poweroff(Meta::MakeIndexSequence<CORES>{});
  //}

  // template <size_t... Is> void poweroff(Meta::IndexSequence<Is...>) {
  //   (arguments_[Is].latch.v(), ...);
  //   (threads_[Is].kill(), ...);
  // }

  template <size_t... Is>
  Meta::Array<CORES, VirtualCPU> create(Meta::IndexSequence<Is...>,
                                        size_t offset) {
    return {VirtualCPU(
        this, Thread::Criterion(Thread::Criterion::NORMAL,
                                (offset + Is) % Traits<CPU>::Active))...};
  }

  void boot(size_t core, void *entry, void *opaque) override {
    cpus_[core].boot(core, entry, opaque);
  }

  bool read(uintptr_t address, void *destination, size_t length) override {
    bool handled = false;

    Meta::for_each(devices_, [&](auto &device) {
      if (!handled)
        handled = device.read(address, destination, length);
    });

    return handled;
  }

  bool write(uintptr_t address, const void *source, size_t length) override {
    bool handled = false;

    Meta::for_each(devices_, [&](auto &device) {
      if (!handled)
        handled = device.write(address, source, length);
    });

    return handled;
  }

  void interrupt(size_t id) override {
    bool handled = false;

    Meta::for_each(devices_, [&](auto &device) {
      using Device = Meta::RemoveReference<decltype(device)>::Result;
      if constexpr (IsVirtualInterruptController<Device>::Result) {
        device.interrupt(id);
        handled = true;
      }
    });

    assert(handled);
  }

  VirtualCPU &cpu(size_t id) override {
    assert(id < CORES);
    return cpus_[id];
  }

  size_t cpus() const override { return CORES; }

private:
  Meta::Array<CORES, VirtualCPU> cpus_;
  Meta::Tuple<DEVICES...> devices_;
};

} // namespace QUARK

#endif
