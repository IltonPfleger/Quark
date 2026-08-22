#pragma once

#include <Traits.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>
#include <hypervisor/VirtualInterruptController.hpp>
#include <hypervisor/VirtualMachine.hpp>
#include <utility/Atomic.hpp>

namespace QUARK {

template <size_t CORES, uintptr_t ADDRESS>
class VirtualPLIC : public VirtualInterruptController {
  enum {
    PRIORITY = 0x000000,
    PENDING = 0x001000,
    ENABLED = 0x002000,
    THRESHOLD = 0x200000,
    CLAIM = 0x200004,
  };

public:
  VirtualPLIC(VirtualMachine &owner) : owner_(owner) {}

  bool pending(uint32_t context) const {
    if (context >= CORES)
      return false;

    for (uint32_t bank = 0; bank < 32; ++bank) {
      uint32_t active = pendings[bank] & enables[context][bank];
      if (bank == 0)
        active &= ~1U;

      while (active) {
        uint32_t bit = __builtin_ctz(active);
        uint32_t interrupt = (bank << 5) | bit;
        if (priorities[interrupt] > thresholds[context])
          return true;
        active &= ~(1U << bit);
      }
    }

    return false;
  }

  bool active(size_t core) const {
    if (core >= CORES)
      return false;

    return pending(core);
  }

  bool pending() const {
    for (size_t core = 0; core < CORES; ++core) {
      if (active(core))
        return true;
    }
    return false;
  }

  void interrupt(size_t identifier) {
    if (identifier == 0 || identifier >= 1024)
      return;

    uint32_t bank = identifier >> 5;
    uint32_t bit = identifier & 31;
    uint32_t mask = 1U << bit;

    pendings[bank] |= mask;

    for (uint32_t context = 0; context < CORES; ++context) {
      if (!(enables[context][bank] & mask))
        continue;
      if (priorities[identifier] <= thresholds[context])
        continue;
      notify(context);
    }
  }

  uint32_t claim(uint32_t context) {
    if (context >= CORES)
      return 0;

    uint32_t limit = thresholds[context];
    uint32_t best = 0;

    for (uint32_t bank = 0; bank < 32; ++bank) {
      uint32_t active = pendings[bank] & enables[context][bank];
      if (bank == 0)
        active &= ~1U;

      while (active) {
        uint32_t bit = __builtin_ctz(active);
        uint32_t interrupt = (bank << 5) | bit;

        if (priorities[interrupt] > limit) {
          limit = priorities[interrupt];
          best = interrupt;
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

  bool read(uintptr_t address, void *pointer, size_t length) {
    assert(length == sizeof(uint32_t));

    uint32_t *const destination = reinterpret_cast<uint32_t *>(pointer);
    const size_t offset = address - ADDRESS;

    if (offset < PENDING) {
      return priority(offset, destination);
    } else if (offset >= ENABLED && offset < THRESHOLD) {
      return enable(offset, destination);
    } else if (offset >= THRESHOLD) {
      return control(offset, destination);
    }

    return false;
  }

  bool write(uintptr_t address, const void *pointer, size_t length) {
    assert(length == sizeof(uint32_t));

    const uint32_t source = *reinterpret_cast<const uint32_t *>(pointer);
    const size_t offset = address - ADDRESS;

    if (offset < PENDING) {
      return priority(offset, source);
    } else if (offset >= ENABLED && offset < THRESHOLD) {
      return enable(offset, source);
    } else if (offset >= THRESHOLD) {
      return control(offset, source);
    }

    return false;
  }

private:
  void notify(uint32_t context) { owner_.cpu(context).setInterruptPending(); }

  void update(size_t core) {
    if (!active(core)) {
      owner_.cpu(core).clearInterruptPending();
    }
  }

  bool priority(size_t offset, uint32_t *output) const {
    uint32_t interrupt = offset / 4;
    if (interrupt >= 1024)
      return false;
    *output = priorities[interrupt];
    return true;
  }

  bool priority(size_t offset, uint32_t source) {
    uint32_t interrupt = offset / 4;
    if (interrupt >= 1024)
      return false;
    priorities[interrupt] = source;
    return true;
  }

  bool enable(size_t offset, uint32_t *const destination) const {
    offset = offset - ENABLED;
    uint32_t context = offset / 0x80;
    uint32_t chunk = (offset % 0x80) / 4;

    if (context >= CORES || chunk >= 32)
      return false;

    *destination = enables[context][chunk];
    return true;
  }

  bool enable(size_t offset, uint32_t source) {
    offset = offset - ENABLED;
    uint32_t context = offset / 0x80;
    uint32_t chunk = (offset % 0x80) / 4;

    if (context >= CORES || chunk >= 32)
      return false;

    enables[context][chunk] = source;
    return true;
  }

  bool control(size_t offset, uint32_t *output) {
    size_t relative = offset - THRESHOLD;
    uint32_t context = relative / 0x1000;
    uint32_t target = relative % 0x1000;

    if (context >= CORES)
      return false;

    if (target == 0) {
      *output = thresholds[context];
      return true;
    }
    if (target == 4) {
      *output = claim(context);
      update(context);
      return true;
    }
    return false;
  }

  bool control(size_t offset, uint32_t source) {
    offset = offset - THRESHOLD;
    uint32_t context = offset / 0x1000;
    uint32_t target = offset % 0x1000;

    if (context >= CORES)
      return false;

    if (target == 0) {
      thresholds[context] = source;
      return true;
    }
    if (target == 4) {
      return true;
    }
    return false;
  }

  uint32_t priorities[1024]{};
  uint32_t pendings[32]{};
  uint32_t enables[CORES][32]{};
  uint32_t thresholds[CORES]{};
  VirtualMachine &owner_;
};

} // namespace QUARK
