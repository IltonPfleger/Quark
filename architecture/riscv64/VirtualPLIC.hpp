#pragma once

#include <Traits.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>
#include <hypervisor/VirtualInterruptController.hpp>
#include <utility/Atomic.hpp>

namespace QUARK {

template <uintptr_t Address> class VirtualPLIC : public VirtualInterruptController {
    enum {
        PRIORITY  = 0x000000,
        PENDING   = 0x001000,
        ENABLED   = 0x002000,
        THRESHOLD = 0x200000,
        CLAIM     = 0x200004,
    };

  public:
    VirtualPLIC(VirtualCPU &cpu)
        : cpu_(cpu) {}

    bool pending() const {
        for (uint32_t context = 0; context < NumberOfContexts; ++context) {
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
        }
        return false;
    }

    void interrupt(uint32_t id) {
        uint32_t bank = id >> 5;
        uint32_t bit  = id & 31;
        uint32_t mask = (1U << bit);

        for (uint32_t c = 0; c < NumberOfContexts; ++c) {
            if (enabled_[c][bank] & mask) {
                pending_[bank] |= mask;
                cpu_.setInterruptPending();
                break;
            }
        }
    }

    uint32_t claim(uint32_t context) {
        for (uint32_t i = 0; i < 32; ++i) {
            uint32_t active = pending_[i] & enabled_[context][i];

            if (i == 0) active &= ~1U;

            if (active) {
                uint32_t bit = static_cast<uint32_t>(__builtin_ctz(active));
                pending_[i] &= ~(1U << bit);
                return (i << 5) | bit;
            }
        }
        return 0;
    }

    bool read(uintptr_t address, unsigned int *destination) {
        size_t offset = address - Address;
        if (offset >= ENABLED && offset < THRESHOLD) {
            unsigned int context = (offset - ENABLED) / 0x80;
            unsigned int chunk   = ((offset - ENABLED) % 0x80) / 4;
            *destination         = enabled_[context][chunk];
            return true;
        } else if (offset >= THRESHOLD) {
            unsigned int off     = offset - THRESHOLD;
            unsigned int context = off / 0x1000;
            unsigned int reg     = off % 0x1000;

            if (context >= NumberOfContexts) return false;

            if (reg == 0) {
                *destination = threshold_[context];
                return true;
            } else if (reg == 4) {
                *destination = claim(context);
                if (!pending()) cpu_.clearInterruptPending();
                return true;
            }
        }
        return false;
    }

    bool write(uintptr_t address, unsigned int source) {
        size_t offset = address - Address;
        if (offset >= PRIORITY && offset < PENDING) {
            priority_[offset / 4] = source;
            return true;
        } else if (offset >= ENABLED && offset < THRESHOLD) {
            offset -= ENABLED;
            unsigned int context     = offset / 0x80;
            unsigned int chunk       = (offset % 0x80) / 4;
            enabled_[context][chunk] = source;
            return true;
        } else if (offset >= THRESHOLD) {
            unsigned int off     = offset - THRESHOLD;
            unsigned int context = off / 0x1000;
            unsigned int reg     = off % 0x1000;
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
    static constexpr unsigned int NumberOfContexts = 2;
    uint32_t priority_[1024]                       = {0}; // 32 Bits For Each IRQ
    uint32_t pending_[32]                          = {0}; // One Bit For Each IRQ
    uint32_t enabled_[NumberOfContexts][32]        = {0};
    uint32_t threshold_[NumberOfContexts]          = {0}; // Threshold Per Context
    VirtualCPU &cpu_;
};

} // namespace QUARK
