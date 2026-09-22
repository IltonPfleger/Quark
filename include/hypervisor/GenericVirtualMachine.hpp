#ifndef __QUARK_HYPERVISOR_GENERIC_VIRTUAL_MACHINE__
#define __QUARK_HYPERVISOR_GENERIC_VIRTUAL_MACHINE__

#include <Meta.hpp>
#include <Semaphore.hpp>
#include <Thread.hpp>
#include <architecture/VirtualCPU.hpp>
#include <hypervisor/VirtualInterruptController.hpp>
#include <hypervisor/VirtualMachine.hpp>
#include <types.hpp>

namespace QUARK {

template <size_t CORES, typename... DEVICES>
class GenericVirtualMachine : public VirtualMachine {
  struct Arguments {
    constexpr Arguments()
        : cpu(nullptr), semaphore(0), core(0), entry(nullptr), opaque(nullptr) {
    }

    VirtualCPU *cpu;
    Semaphore semaphore;
    size_t core;
    void *entry;
    void *opaque;
  };

public:
  GenericVirtualMachine(void *entry, size_t size, size_t offset)
      : VirtualMachine(Chunk(entry, size)), arguments_(),
        cpus_(cpus(Meta::MakeIndexSequence<CORES>{})), devices_(*this),
        threads_(threads(Meta::MakeIndexSequence<CORES>{}, offset)) {}

  template <size_t... Is>
  Meta::Array<CORES, VirtualCPU> cpus(Meta::IndexSequence<Is...>) {
    return {((void)Is, VirtualCPU(this))...};
  }

  template <size_t... Is>
  Meta::Array<CORES, Thread> threads(Meta::IndexSequence<Is...>,
                                     size_t offset) {
    return {Thread(worker, &arguments_[Is],
                   Thread::Criterion(Thread::Criterion::NORMAL,
                                     (offset + Is) % Traits<CPU>::Active))...};
  }

  void boot(size_t core, void *entry, void *opaque) {
    arguments_[core].cpu = &cpus_[core];
    arguments_[core].core = core;
    arguments_[core].entry = entry;
    arguments_[core].opaque = opaque;
    arguments_[core].semaphore.v();
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

  static void *worker(void *pointer) {
    Arguments *arguments = reinterpret_cast<Arguments *>(pointer);
    arguments->semaphore.p();
    arguments->cpu->boot(arguments->core, arguments->entry, arguments->opaque);
    return nullptr;
  };

private:
  Arguments arguments_[CORES];
  Meta::Array<CORES, VirtualCPU> cpus_;
  Meta::Tuple<DEVICES...> devices_;
  Meta::Array<CORES, Thread> threads_;
};

} // namespace QUARK

#endif
