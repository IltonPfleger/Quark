#pragma once

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/sbi/values.hpp>

namespace QUARK::sbi {

class FWFT {
public:
  static constexpr unsigned int EID = 0x46574654;

  static void handler(ContextFrame *context) {
    context->a0 = SBI_ERR_NOT_SUPPORTED;
    context->a1 = 0;
  }
};

} // namespace QUARK::sbi
