#pragma once

#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>

namespace QUARK {

namespace sbi {

class Time {
public:
  static constexpr unsigned int EID = 'T' << 24 | 'I' << 16 | 'M' << 8 | 'E';

  static void handler(ContextFrame *c) {
    VirtualCPU::mtimecmp(c->a0);
    c->a0 = 0;
    c->a1 = 0;
  }
};

} // namespace sbi

} // namespace QUARK
