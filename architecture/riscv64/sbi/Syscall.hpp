#pragma once

#include <architecture/riscv64/sbi/Base.hpp>
#include <architecture/riscv64/sbi/Counter.hpp>
#include <architecture/riscv64/sbi/FWFT.hpp>
#include <architecture/riscv64/sbi/HSM.hpp>
#include <architecture/riscv64/sbi/RFNC.hpp>
#include <architecture/riscv64/sbi/Time.hpp>

namespace QUARK::sbi {

class Syscall {
  public:
    static constexpr unsigned int CODE = 9;
    static void dispatch(ContextFrame *context) {
        switch (context->a7) {
            case Base::EID: Base::handler(context); break;
            case Time::EID: Time::handler(context); break;
            case Counter::EID: Counter::handler(context); break;
            case FWFT::EID: FWFT::handler(context); break;
            case HSM::EID: HSM::handler(context); break;
            case RFNC::EID: RFNC::handler(context); break;
            default: ExceptionHandler::esr(context);
        }
        context->pc += 4;
    }
};

} // namespace QUARK::sbi
