#pragma once

#include <Traits.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>
#include <hypervisor/VirtualInterruptController.hpp>
#include <utility/Atomic.hpp>

namespace QUARK {

template <size_t CORES, uintptr_t ADDRESS>
class VirtualPLIC : public VirtualInterruptController {
  static constexpr size_t Factor = 2;
  static constexpr size_t Contexts = CORES * Factor;

  enum {
    PRIORITY = 0x000000,
    PENDING = 0x001000,
    ENABLED = 0x002000,
    THRESHOLD = 0x200000,
    CLAIM = 0x200004,
  };

public:
  VirtualPLIC(Meta::Array<CORES, VirtualCPU> &cpus) : cpus(cpus) {}

  bool pending(uint32_t context) const {
    if (context >= Contexts)
      return false;

    for (uint32_t bank = 0; bank < 32; ++bank) {
      uint32_t active = pendings[bank] & enables[context][bank];
      if (bank == 0)
        active &= ~1U;

      while (active) {
        uint32_t bit = __builtin_ctz(active);
        uint32_t irq = (bank << 5) | bit;
        if (priorities[irq] > thresholds[context])
          return true;
        active &= ~(1U << bit);
      }
    }
    return false;
  }

  bool active(size_t core) const {
    if (core >= CORES)
      return false;
    for (size_t ctx = 0; ctx < Factor; ++ctx) {
      if (pending(core * Factor + ctx))
        return true;
    }
    return false;
  }

  bool pending() const {
    for (size_t core = 0; core < CORES; ++core) {
      if (active(core))
        return true;
    }
    return false;
  }

  void interrupt(uint32_t id) {
    if (id == 0 || id >= 1024)
      return;

    uint32_t bank = id >> 5;
    uint32_t bit = id & 31;
    uint32_t mask = 1U << bit;

    pendings[bank] |= mask;

    for (uint32_t context = 0; context < Contexts; ++context) {
      if (!(enables[context][bank] & mask))
        continue;
      if (priorities[id] <= thresholds[context])
        continue;
      notify(context);
    }
  }

  uint32_t claim(uint32_t context) {
    if (context >= Contexts)
      return 0;

    uint32_t limit = thresholds[context];
    uint32_t best = 0;

    for (uint32_t bank = 0; bank < 32; ++bank) {
      uint32_t active = pendings[bank] & enables[context][bank];
      if (bank == 0)
        active &= ~1U;

      while (active) {
        uint32_t bit = __builtin_ctz(active);
        uint32_t irq = (bank << 5) | bit;

        if (priorities[irq] > limit) {
          limit = priorities[irq];
          best = irq;
        }
        active &= ~(1U << bit);
      }
    }

    if (best != 0) {
      uint32_t bank = best >> 5;
      uint32_t bit = best & 31;
      pendings[bank] &= ~(1U << bit);
    }

    return best;
  }

  bool read(uintptr_t address, unsigned int *out) {
    size_t offset = address - ADDRESS;

    if (offset < PENDING) {
      return priority(offset, out);
    }
    if (offset >= ENABLED && offset < THRESHOLD) {
      return enable(offset, out);
    }
    if (offset >= THRESHOLD) {
      return control(offset, out);
    }
    return false;
  }

  bool write(uintptr_t address, unsigned int val) {
    size_t offset = address - ADDRESS;

    if (offset < PENDING) {
      return priority(offset, val);
    }
    if (offset >= ENABLED && offset < THRESHOLD) {
      return enable(offset, val);
    }
    if (offset >= THRESHOLD) {
      return control(offset, val);
    }
    return false;
  }

private:
  void notify(uint32_t context) {
    size_t core = context / Factor;
    cpus[core].setInterruptPending();
  }

  void update(size_t core) {
    if (!active(core)) {
      cpus[core].clearInterruptPending();
    }
  }

  bool priority(size_t offset, unsigned int *out) const {
    uint32_t irq = offset / 4;
    if (irq >= 1024)
      return false;
    *out = priorities[irq];
    return true;
  }

  bool priority(size_t offset, unsigned int val) {
    uint32_t irq = offset / 4;
    if (irq >= 1024)
      return false;
    priorities[irq] = val;
    return true;
  }

  bool enable(size_t offset, unsigned int *out) const {
    size_t off = offset - ENABLED;
    uint32_t context = off / 0x80;
    uint32_t chunk = (off % 0x80) / 4;

    if (context >= Contexts || chunk >= 32)
      return false;

    *out = enables[context][chunk];
    return true;
  }

  bool enable(size_t offset, unsigned int val) {
    size_t off = offset - ENABLED;
    uint32_t context = off / 0x80;
    uint32_t chunk = (off % 0x80) / 4;

    if (context >= Contexts || chunk >= 32)
      return false;

    enables[context][chunk] = val;
    return true;
  }

  bool control(size_t offset, unsigned int *out) {
    size_t off = offset - THRESHOLD;
    uint32_t context = off / 0x1000;
    uint32_t reg = off % 0x1000;

    if (context >= Contexts)
      return false;

    if (reg == 0) {
      *out = thresholds[context];
      return true;
    }
    if (reg == 4) {
      *out = claim(context);
      size_t core = context / Factor;
      update(core);
      return true;
    }
    return false;
  }

  bool control(size_t offset, unsigned int val) {
    size_t off = offset - THRESHOLD;
    uint32_t context = off / 0x1000;
    uint32_t reg = off % 0x1000;

    if (context >= Contexts)
      return false;

    if (reg == 0) {
      thresholds[context] = val;
      return true;
    }
    if (reg == 4) {
      return true;
    }
    return false;
  }

  uint32_t priorities[1024]{};
  uint32_t pendings[32]{};
  uint32_t enables[Contexts][32]{};
  uint32_t thresholds[Contexts]{};
  Meta::Array<CORES, VirtualCPU> &cpus;
};

} // namespace QUARK
