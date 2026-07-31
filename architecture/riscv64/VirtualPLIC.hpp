#pragma once

#include <Traits.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>
#include <hypervisor/VirtualInterruptController.hpp>
#include <utility/Atomic.hpp>

namespace QUARK {

template <size_t CORES, uintptr_t ADDRESS> class VirtualPLIC : public VirtualInterruptController {
    static constexpr size_t ContextsPerCore = 2;
    static constexpr size_t Contexts        = CORES * ContextsPerCore;

    enum {
        PRIORITY  = 0x000000,
        PENDING   = 0x001000,
        ENABLED   = 0x002000,
        THRESHOLD = 0x200000,
        CLAIM     = 0x200004,
    };

  public:
    VirtualPLIC(Meta::Array<CORES, VirtualCPU> &cpus)
        : cpus_(cpus) {}

    bool pending(uint32_t context) const {
        if (context >= Contexts) return false;

        for (uint32_t bank = 0; bank < 32; ++bank) {
            uint32_t active = pending_[bank] & enabled_[context][bank];
            if (bank == 0) active &= ~1U;

            while (active) {
                uint32_t bit = __builtin_ctz(active);
                uint32_t irq = (bank << 5) | bit;
                if (priority_[irq] > threshold_[context]) return true;
                active &= ~(1U << bit);
            }
        }
        return false;
    }

    bool active(size_t core) const {
        if (core >= CORES) return false;
        for (size_t ctx = 0; ctx < ContextsPerCore; ++ctx) {
            if (pending(core * ContextsPerCore + ctx)) return true;
        }
        return false;
    }

    bool pending() const {
        for (size_t core = 0; core < CORES; ++core) {
            if (active(core)) return true;
        }
        return false;
    }

    void interrupt(uint32_t id) {
        if (id == 0 || id >= 1024) return;

        uint32_t bank = id >> 5;
        uint32_t bit  = id & 31;
        uint32_t mask = 1U << bit;

        pending_[bank] |= mask;

        for (uint32_t context = 0; context < Contexts; ++context) {
            if (!(enabled_[context][bank] & mask)) continue;
            if (priority_[id] <= threshold_[context]) continue;

            uint32_t core = context / ContextsPerCore;
            cpus_[core].setInterruptPending();
        }
    }

    uint32_t claim(uint32_t context) {
        if (context >= Contexts) return 0;

        uint32_t max_priority = threshold_[context];
        uint32_t best_irq     = 0;

        for (uint32_t bank = 0; bank < 32; ++bank) {
            uint32_t active = pending_[bank] & enabled_[context][bank];
            if (bank == 0) active &= ~1U;

            while (active) {
                uint32_t bit = __builtin_ctz(active);
                uint32_t irq = (bank << 5) | bit;

                if (priority_[irq] > max_priority) {
                    max_priority = priority_[irq];
                    best_irq     = irq;
                }
                active &= ~(1U << bit);
            }
        }

        if (best_irq != 0) {
            uint32_t bank = best_irq >> 5;
            uint32_t bit  = best_irq & 31;
            pending_[bank] &= ~(1U << bit);
        }

        return best_irq;
    }

    bool read(uintptr_t address, unsigned int *destination) {
        size_t offset = address - ADDRESS;

        if (offset >= PRIORITY && offset < PENDING) {
            uint32_t irq = offset / 4;
            if (irq < 1024) {
                *destination = priority_[irq];
                return true;
            }
        } else if (offset >= ENABLED && offset < THRESHOLD) {
            unsigned int context = (offset - ENABLED) / 0x80;
            unsigned int chunk   = ((offset - ENABLED) % 0x80) / 4;

            if (context < Contexts && chunk < 32) {
                *destination = enabled_[context][chunk];
                return true;
            }
        } else if (offset >= THRESHOLD) {
            unsigned int off     = offset - THRESHOLD;
            unsigned int context = off / 0x1000;
            unsigned int reg     = off % 0x1000;

            if (context >= Contexts) return false;

            if (reg == 0) {
                *destination = threshold_[context];
                return true;
            } else if (reg == 4) {
                *destination = claim(context);

                uint32_t core = context / ContextsPerCore;
                if (!active(core)) {
                    cpus_[core].clearInterruptPending();
                }
                return true;
            }
        }
        return false;
    }

    bool write(uintptr_t address, unsigned int source) {
        size_t offset = address - ADDRESS;

        if (offset >= PRIORITY && offset < PENDING) {
            uint32_t irq = offset / 4;
            if (irq < 1024) {
                priority_[irq] = source;
                return true;
            }
        } else if (offset >= ENABLED && offset < THRESHOLD) {
            unsigned int context = (offset - ENABLED) / 0x80;
            unsigned int chunk   = ((offset - ENABLED) % 0x80) / 4;

            if (context < Contexts && chunk < 32) {
                enabled_[context][chunk] = source;
                return true;
            }
        } else if (offset >= THRESHOLD) {
            unsigned int off     = offset - THRESHOLD;
            unsigned int context = off / 0x1000;
            unsigned int reg     = off % 0x1000;

            if (context >= Contexts) return false;

            if (reg == 0) {
                threshold_[context] = source;
                return true;
            } else if (reg == 4) {
                return true;
            }
        }
        return false;
    }

  private:
    uint32_t priority_[1024]        = {0};
    uint32_t pending_[32]           = {0};
    uint32_t enabled_[Contexts][32] = {0};
    uint32_t threshold_[Contexts]   = {0};
    Meta::Array<CORES, VirtualCPU> &cpus_;
};

} // namespace QUARK
