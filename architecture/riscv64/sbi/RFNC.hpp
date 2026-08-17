#ifndef __QUARK_RISCV64_SBI_RFNC__
#define __QUARK_RISCV64_SBI_RFNC__

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/ExceptionHandler.hpp>

namespace QUARK::sbi {

// TODO: Send IPI to Target Harts Specified by (a0) and (a1)

class RFNC {
private:
  static constexpr uintptr_t PageSize = Traits<Memory>::PageSize;

  static void sfence(uintptr_t start, size_t size, uintptr_t asid = 0,
                     bool has = false) {
    if (start == 0 || size == 0 || size == static_cast<uintptr_t>(-1)) {
      if (has) {
        asm volatile("sfence.vma zero, %0" ::"r"(asid) : "memory");
      } else {
        asm volatile("sfence.vma" ::: "memory");
      }
      return;
    }

    uintptr_t end = start + size;
    for (uintptr_t address = start; address < end; address += PageSize) {
      if (has) {
        asm volatile("sfence.vma %0, %1" ::"r"(address), "r"(asid) : "memory");
      } else {
        asm volatile("sfence.vma %0, zero" ::"r"(address) : "memory");
      }
    }
  }

public:
  static constexpr unsigned int EID = 'R' << 24 | 'F' << 16 | 'N' << 8 | 'C';

  static void handler(ContextFrame *context) {
    uintptr_t start = context->a2;
    uintptr_t size = context->a3;
    uintptr_t asid = context->a4;

    switch (context->a6) {
    case 0: {
      asm volatile("fence.i" ::: "memory");
      context->a0 = 0;
      break;
    }

    case 1: {
      sfence(start, size);
      context->a0 = 0;
      break;
    }

    case 2: {
      sfence(start, size, asid, true);
      context->a0 = 0;
      break;
    }

    default:
      ExceptionHandler::esr(context);
      break;
    }
  }
};

} // namespace QUARK::sbi

#endif
