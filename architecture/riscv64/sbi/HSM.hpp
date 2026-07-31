#ifndef __QUARK_RISCV64_SBI_HSM__
#define __QUARK_RISCV64_SBI_HSM__

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/ExceptionHandler.hpp>

namespace QUARK::sbi {

class HSM {
  public:
    static constexpr unsigned int EID = 'H' << 16 | 'S' << 8 | 'M';

    static void handler(ContextFrame *context) {
        switch (context->a6) {
            case 0: {
                uint64_t a0 = context->a0;
                void *a1    = reinterpret_cast<void *>(context->a1);
                void *a2    = reinterpret_cast<void *>(context->a2);

                context->a0 = 0;

                VirtualCPU::kick(a0, a1, a2);

                break;
            }
            default: ExceptionHandler::esr(context);
        }
    }
};

} // namespace QUARK::sbi

#endif
