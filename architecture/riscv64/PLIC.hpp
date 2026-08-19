#pragma once

#include <architecture/riscv64/CPU.hpp>
#include <drivers/Driver.hpp>

namespace QUARK {

class PLIC : Driver {
  enum {
    PRIORITY = 0x000000,
    PENDING = 0x001000,
    ENABLED = 0x002000,
    THRESHOLD = 0x200000,
    CLAIM = 0x200004,
  };

public:
  static void priority(unsigned int source, unsigned int value) {
    Reg32(Address, PRIORITY + source * 4) = value;
  }

  static void threshold(unsigned int context, unsigned int value) {
    Reg32(Address, THRESHOLD + (context * 0x1000)) = value;
  }

  static unsigned int claim(unsigned int c = context()) {
    return Reg32(Address, CLAIM + (c * 0x1000));
  }
  static void complete(unsigned int id, unsigned int c = context()) {
    Reg32(Address, CLAIM + (c * 0x1000)) = id;
  }

  static unsigned int context() {
    return Traits<PLIC>::Contexts[CPU::id() + Traits<CPU>::Offset]
                                 [Traits<RISCV>::Supervisor];
  }

  static void enable(unsigned int source, unsigned int c = context()) {
    unsigned int bank = source / 32;
    unsigned int bit = source % 32;
    Reg32(Address, ENABLED + (c * 0x80) + (bank * 4)) |= (1 << bit);
  }

  static void disable(unsigned int source, unsigned int c = context()) {
    unsigned int bank = source / 32;
    unsigned int bit = source % 32;
    Reg32(Address, ENABLED + (c * 0x80) + (bank * 4)) &= ~(1 << bit);
  }

  static void init() {
    threshold(context(), 0);

    for (unsigned int i = 0; i < Traits<PLIC>::NumberOfInterruptions; i++) {
      priority(i, 1);
      disable(i);
    }
  }

private:
  static constexpr unsigned long Address = Traits<MemoryMap>::PLIC;
};

} // namespace QUARK
