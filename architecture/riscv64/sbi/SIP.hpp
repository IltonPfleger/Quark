#ifndef __QUARK_RISCV64_SBI_SIP__
#define __QUARK_RISCV64_SBI_SIP__

#include <architecture/riscv64/ExceptionHandler.hpp>
#include <architecture/riscv64/IPI.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>

namespace QUARK::sbi {

class SIP {
public:
  static constexpr unsigned int EID = 's' << 16 | 'P' << 8 | 'I';

  static void handler(ContextFrame *context) {
    if (context->a6 == 0) {
      uintmax_t harts = context->a0;
      uintmax_t base = context->a1;

      for (unsigned int bit = 0; bit < sizeof(uintptr_t) * 8; ++bit) {
        uintmax_t mask = 1ULL << bit;

        if (harts & mask) {
          VirtualCPU::interProcessorInterrupt(base + bit);
          harts &= ~mask;
        }

        if (!harts)
          break;
      }

      context->a0 = 0;

    } else {
      ExceptionHandler::esr(context);
    }
  }
};

} // namespace QUARK::sbi

#endif
