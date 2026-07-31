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

template <size_t CORES, typename... Devices> class GenericVirtualMachine : public VirtualMachine {

    class Arguments {
      public:
        Arguments()
            : cpu(nullptr),
              semaphore(0),
              core(0),
              entry(nullptr),
              opaque(nullptr) {}

        VirtualCPU *cpu;
        Semaphore semaphore;
        size_t core;
        void *entry;
        void *opaque;
    };

    template <typename... D> struct DeviceCollection {
        DeviceCollection(VirtualMachine &, Meta::Array<CORES, VirtualCPU> &) {}
        bool read(uintptr_t, uint32_t *) { return false; }
        bool write(uintptr_t, uint32_t) { return false; }
        void interrupt(uint32_t) {}
    };

    template <typename Head, typename... Tail> struct DeviceCollection<Head, Tail...> {
        Head _device;
        DeviceCollection<Tail...> _others;

        DeviceCollection(VirtualMachine &machine, Meta::Array<CORES, VirtualCPU> &cpus)
            : _device(create(machine, cpus)),
              _others(machine, cpus) {}

        static Head create(VirtualMachine &machine, Meta::Array<CORES, VirtualCPU> &cpus) {
            if constexpr (Meta::IsBaseOf<VirtualInterruptController, Head>::Result) {
                return Head(cpus);
            } else {
                return Head(machine);
            }
        }

        bool read(uintptr_t target, uint32_t *destination) {
            if (_device.read(target, destination)) {
                return true;
            }
            return _others.read(target, destination);
        }

        bool write(uintptr_t target, uint32_t source) {
            if (_device.write(target, source)) {
                return true;
            }
            return _others.write(target, source);
        }

        void interrupt(uint32_t id) {
            if constexpr (Meta::IsBaseOf<VirtualInterruptController, Head>::Result) {
                _device.interrupt(id);
            } else {
                _others.interrupt(id);
            }
        }
    };

  public:
    GenericVirtualMachine(void *entry, size_t size)
        : VirtualMachine(Chunk(entry, size)),
          cpus_(cpus(Meta::MakeIndexSequence<CORES>{})),
          devices_(*this, cpus_),
          threads_(threads(Meta::MakeIndexSequence<CORES>{})) {}

    template <size_t... Is> Meta::Array<CORES, VirtualCPU> cpus(Meta::IndexSequence<Is...>) { return {((void)Is, VirtualCPU(this))...}; }
    template <size_t... Is> Meta::Array<CORES, Thread> threads(Meta::IndexSequence<Is...>) { return {(Thread(worker, &arguments[Is]))...}; }

    void boot(size_t core, void *entry, void *opaque) {
        arguments[core].cpu    = &cpus_[core];
        arguments[core].core   = core;
        arguments[core].entry  = entry;
        arguments[core].opaque = opaque;
        arguments[core].semaphore.v();
    }

    bool read(uintptr_t target, uint32_t *destination) override { return devices_.read(target, destination); }

    bool write(uintptr_t target, uint32_t source) override { return devices_.write(target, source); }

    void interrupt(uint32_t id) override { devices_.interrupt(id); }

    static void *worker(void *pointer) {
        Arguments *arguments = reinterpret_cast<Arguments *>(pointer);
        arguments->semaphore.p();
        arguments->cpu->boot(arguments->core, arguments->entry, arguments->opaque);
        return nullptr;
    };

  private:
    Arguments arguments[CORES];
    Meta::Array<CORES, VirtualCPU> cpus_;
    DeviceCollection<Devices...> devices_;
    Meta::Array<CORES, Thread> threads_;
};

} // namespace QUARK

#endif
