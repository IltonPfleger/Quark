#ifndef __QUARK_RISCV64_SBI_RFNC__
#define __QUARK_RISCV64_SBI_RFNC__

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/ExceptionHandler.hpp>
#include <architecture/riscv64/IPI.hpp>

// TODO: VMs can't sent ipi to real harts

namespace QUARK::sbi {

class RFNC {
private:
  static constexpr uintptr_t PageSize = Traits<Memory>::PageSize;

  static void sfence(uintptr_t start, size_t size, uintptr_t asid = 0,
                     bool has = false) {
    if (start == 0 || size == 0 || size == static_cast<uintptr_t>(-1)) {
      if (has) {
        asm("sfence.vma zero, %0" ::"r"(asid) : "memory");
      } else {
        asm("sfence.vma" ::: "memory");
      }
      return;
    }

    uintptr_t end = start + size;
    for (uintptr_t address = start; address < end; address += PageSize) {
      if (has) {
        asm("sfence.vma %0, %1" ::"r"(address), "r"(asid) : "memory");
      } else {
        asm("sfence.vma %0, zero" ::"r"(address) : "memory");
      }
    }
  }

public:
  static constexpr unsigned int EID = 'R' << 24 | 'F' << 16 | 'N' << 8 | 'C';

  static bool worker(const uintptr_t *arguments) {
    uintptr_t start = arguments[2];
    uintptr_t size = arguments[3];
    uintptr_t asid = arguments[4];
    uintptr_t call = arguments[6];

    switch (call) {
    case 0: {
      asm("fence.i" ::: "memory");
      return true;
    }

    case 1: {
      sfence(start, size);
      return true;
      break;
    }

    case 2: {
      sfence(start, size, asid, true);
      return true;
    }

    default:
      return false;
    }
  }

  static void adapter(void *pointer) {
    const uintptr_t *arguments = reinterpret_cast<const uintptr_t *>(pointer);
    worker(arguments);
  }

  static void ipi(uintptr_t *arguments) { worker(arguments); }

  static void handler(ContextFrame *context) {
    uintptr_t mask = context->a0;
    uintptr_t base = context->a1;

    for (unsigned int bit = 0; bit < sizeof(uintptr_t) * 8; ++bit) {
      if (mask & (uintptr_t(1) << bit)) {
        size_t hartid = base + bit;

        assert(hartid < Traits<CPU>::Count);

        uintptr_t a0 = context->a0;
        uintptr_t a1 = context->a1;
        uintptr_t a2 = context->a2;
        uintptr_t a3 = context->a3;
        uintptr_t a4 = context->a4;
        uintptr_t a5 = context->a5;
        uintptr_t a6 = context->a6;
        uintptr_t a7 = context->a7;

        IPI::send(hartid, adapter, a0, a1, a2, a3, a4, a5, a6, a7);
      }
    }

    if (worker(&context->a0)) {
      context->a0 = 0;
    } else {
      ExceptionHandler::esr(context);
    }
  }
};

} // namespace QUARK::sbi

#endif
