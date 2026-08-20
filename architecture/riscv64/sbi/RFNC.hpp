#ifndef __QUARK_RISCV64_SBI_RFNC__
#define __QUARK_RISCV64_SBI_RFNC__

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/ExceptionHandler.hpp>
#include <architecture/riscv64/IPI.hpp>

// TODO: VMs can't sent ipi to real harts

namespace QUARK::sbi {

class RFNC {
public:
  static constexpr unsigned int EID = 'R' << 24 | 'F' << 16 | 'N' << 8 | 'C';

  static void handler(ContextFrame *context) {
    uintptr_t mask = context->a0;
    uintptr_t base = context->a1;

    auto all = [&](auto action) {
      for (unsigned int bit = 0; bit < sizeof(uintptr_t) * 8; ++bit) {
        if (mask & (uintptr_t(1) << bit)) {
          size_t hartid = base + bit;
          assert(hartid < Traits<CPU>::Count);
          action(hartid);
        }
      }
    };

    switch (context->a6) {
    case 0:
      all([](size_t hartid) {
        VirtualCPU::fence(hartid, VirtualCPU::PENDING_FENCE_I);
      });
      context->a0 = SBI_SUCCESS;
      break;
    case 1:
    case 2:
      all([&](size_t hartid) {
        VirtualCPU::fence(hartid, VirtualCPU::PENDING_SFENCE);
      });
      context->a0 = SBI_SUCCESS;
      break;
    default:
      context->a0 = SBI_ERR_NOT_SUPPORTED;
      break;
    }
  }
};

} // namespace QUARK::sbi

#endif
