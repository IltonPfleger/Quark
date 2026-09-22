#pragma once

#include <architecture/CLINT.hpp>
#include <architecture/ContextFrame.hpp>
#include <architecture/VirtualCPU.hpp>

namespace QUARK {

namespace sbi {

class Time {
public:
  static constexpr unsigned int EID = 'T' << 24 | 'I' << 16 | 'M' << 8 | 'E';

  static void handler(ContextFrame *context) {
    VirtualCPU::mtimecmp(context->a0);
    context->a0 = SBI_SUCCESS;
    context->a1 = 0;
  }
};

} // namespace sbi

} // namespace QUARK
