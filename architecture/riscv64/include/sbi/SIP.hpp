#ifndef __QUARK_RISCV64_SBI_SIP__
#define __QUARK_RISCV64_SBI_SIP__

#include <architecture/ExceptionHandler.hpp>
#include <architecture/IPI.hpp>
#include <architecture/VirtualCPU.hpp>
#include <architecture/sbi/values.hpp>

namespace QUARK::sbi {

class SIP {
public:
  static constexpr unsigned int EID = 's' << 16 | 'P' << 8 | 'I';

  static void handler(ContextFrame *context) {
    switch (context->a6) {
    case 0: {
      uintmax_t harts = context->a0;
      uintmax_t base = context->a1;

      for (unsigned int bit = 0; bit < sizeof(uintmax_t) * 8; ++bit) {
        uintmax_t mask = 1ULL << bit;

        if (harts & mask) {
          if (!VirtualCPU::set_software_interrupt_pending(base + bit)) {
            context->a0 = SBI_ERR_INVALID_PARAM;
            context->a1 = 0;
            return;
          }
          harts &= ~mask;
        }

        if (!harts)
          break;
      }

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
