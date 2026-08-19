#ifndef __QUARK_RISCV64_SBI_DBCN__
#define __QUARK_RISCV64_SBI_DBCN__

#include <architecture/riscv64/VirtualCPU.hpp>
#include <architecture/riscv64/sbi/values.hpp>

namespace QUARK::sbi {

class DBCN {
public:
  static constexpr unsigned int EID = 'D' << 24 | 'B' << 16 | 'C' << 8 | 'N';

  static void handler(ContextFrame *context) {
    switch (context->a6) {
    case 0: {
      const size_t bytes = context->a0;
      const uintptr_t source = context->a1;

      if (!context->a1 && bytes > 0) {
        context->a0 = SBI_ERR_INVALID_PARAM;
        break;
      }

      for (size_t i = 0; i < bytes; i++) {
        put(source + i);
      }

      context->a0 = SBI_SUCCESS;
      context->a1 = bytes;
      break;
    }
    case 2: {
      put(context->a0 + 1);
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

private:
  static void put(uintptr_t source) {
    uint8_t byte = '\0';
    VirtualCPU::lb(source, &byte);
    Console::print(static_cast<char>(byte));
  }
};

} // namespace QUARK::sbi

#endif
