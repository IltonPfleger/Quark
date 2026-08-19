#ifndef __QUARK_RISCV64_SBI_HSM__
#define __QUARK_RISCV64_SBI_HSM__

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/ExceptionHandler.hpp>
#include <architecture/riscv64/sbi/values.hpp>

namespace QUARK::sbi {

class HSM {
public:
  static constexpr unsigned int EID = 'H' << 16 | 'S' << 8 | 'M';

  static void handler(ContextFrame *context) {
    switch (context->a6) {
    case 0: {
      uint64_t hartid = context->a0;
      void *start = reinterpret_cast<void *>(context->a1);
      void *opaque = reinterpret_cast<void *>(context->a2);

      VirtualCPU::kick(hartid, start, opaque);

      context->a0 = SBI_SUCCESS;
      context->a1 = 0;

      break;
    }

    default: {
      context->a0 = SBI_ERR_NOT_SUPPORTED;
      context->a1 = 0;
      break;
    }
    }
  }
};

} // namespace QUARK::sbi

#endif
