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

  template <bool Write> bool access(size_t offset, uint32_t &value) {
    if (offset < PENDING) {
      size_t identifier = offset / 4;
      if (identifier >= 1024)
        return false;

      if constexpr (Write) {
        priorities_[identifier] = value;
      } else {
        value = priorities_[identifier];
      }
    } else if (offset < ENABLED) {
      size_t bank = (offset - PENDING) / 4;
      if (bank >= 32)
        return false;

      if constexpr (Write) {
        return false;
      } else {
        value = pendings_[bank];
      }
    } else if (offset < THRESHOLD) {
      size_t relative = offset - ENABLED;
      size_t context = relative / 0x80;
      size_t chunk = (relative % 0x80) / 4;

      if (context >= CORES || chunk >= 32)
        return false;

      if constexpr (Write) {
        enables_[context][chunk] = value;
      } else {
        value = enables_[context][chunk];
      }
    } else {
      size_t relative = offset - THRESHOLD;
      size_t context = relative / 0x1000;
      size_t target = relative % 0x1000;

      if (context >= CORES)
        return false;

      if (target == 0) {
        if constexpr (Write) {
          thresholds_[context] = value;
        } else {
          value = thresholds_[context];
        }
      } else if (target == 4) {
        if constexpr (Write) {
          return true;
        } else {
          value = claim(context);
          update(context);
        }
      } else {
        return false;
      }
    }

    return true;
  }

  bool read(uintptr_t address, void *pointer, size_t length) {
    assert(length == sizeof(uint32_t));

    uint32_t *const destination = reinterpret_cast<uint32_t *>(pointer);
    const size_t offset = address - ADDRESS;

    return access<false>(offset, *destination);
  }

  bool write(uintptr_t address, const void *pointer, size_t length) {
    assert(length == sizeof(uint32_t));

    uint32_t source = *reinterpret_cast<const uint32_t *>(pointer);
    const size_t offset = address - ADDRESS;

    return access<true>(offset, source);
  }

  void interrupt(size_t identifier) {
    assert(identifier != 0 && identifier < 1024);

    uint32_t bank = identifier >> 5;
    uint32_t bit = identifier & 31;
    uint32_t mask = 1U << bit;

    pendings_[bank] |= mask;

    for (uint32_t context = 0; context < CORES; ++context) {
      if (!(enables_[context][bank] & mask))
        continue;
      if (priorities_[identifier] <= thresholds_[context])
        continue;
      owner_.cpu(context).set_external_interrupt_pending();
    }
  }

private:
  bool pending(uint32_t context) const {
    if (context >= CORES)
      return false;

    for (uint32_t bank = 0; bank < 32; ++bank) {
      uint32_t active = pendings_[bank] & enables_[context][bank];
      if (bank == 0)
        active &= ~1U;

      while (active) {
        uint32_t bit = __builtin_ctz(active);
        uint32_t interrupt = (bank << 5) | bit;
        if (priorities_[interrupt] > thresholds_[context])
          return true;
        active &= ~(1U << bit);
      }
    }

    return false;
  }

  bool pending() const {
    for (size_t core = 0; core < CORES; ++core) {
      if (pending(core))
        return true;
    }
    return false;
  }

  uint32_t claim(uint32_t context) {
    if (context >= CORES)
      return 0;

    uint32_t limit = thresholds_[context];
    uint32_t best = 0;

    for (uint32_t bank = 0; bank < 32; ++bank) {
      uint32_t active = pendings_[bank] & enables_[context][bank];
      if (bank == 0)
        active &= ~1U;

      while (active) {
        uint32_t bit = __builtin_ctz(active);
        uint32_t interrupt = (bank << 5) | bit;

        if (priorities_[interrupt] > limit) {
          limit = priorities_[interrupt];
          best = interrupt;
        }
        active &= ~(1U << bit);
      }
    }

    if (best != 0) {
      uint32_t bank = best >> 5;
      uint32_t bit = best & 31;
      pendings_[bank] &= ~(1U << bit);
    }

    return best;
  }

  void update(size_t core) {
    if (!pending(core)) {
      owner_.cpu(core).clear_external_interrupt_pending();
    }
  }

  uint32_t priorities_[1024]{};
  uint32_t pendings_[32]{};
  uint32_t enables_[CORES][32]{};
  uint32_t thresholds_[CORES]{};
  VirtualMachine &owner_;
};

} // namespace QUARK
