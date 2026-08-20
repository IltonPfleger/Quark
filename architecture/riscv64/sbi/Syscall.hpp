#pragma once

#include <architecture/riscv64/sbi/Base.hpp>
#include <architecture/riscv64/sbi/Counter.hpp>
#include <architecture/riscv64/sbi/DBCN.hpp>
#include <architecture/riscv64/sbi/FWFT.hpp>
#include <architecture/riscv64/sbi/HSM.hpp>
#include <architecture/riscv64/sbi/RFNC.hpp>
#include <architecture/riscv64/sbi/SIP.hpp>
#include <architecture/riscv64/sbi/Time.hpp>

namespace QUARK::sbi {

class Syscall {
public:
  static constexpr unsigned int CODE = 9;
  static void dispatch(ContextFrame *context) {

    // Console::print((char)(((context->a7 >> 24) & 0xFF)));
    // Console::print((char)(((context->a7 >> 16) & 0xFF)));
    // Console::print((char)(((context->a7 >> 8) & 0xFF)));
    // Console::println((char)(((context->a7) & 0xFF)));

    switch (context->a7) {
    case Base::EID:
      Base::handler(context);
      break;
    case Time::EID:
      Time::handler(context);
      break;
    case Counter::EID:
      Counter::handler(context);
      break;
    case FWFT::EID:
      FWFT::handler(context);
      break;
    case HSM::EID:
      HSM::handler(context);
      break;
    // case RFNC::EID:
    //   RFNC::handler(context);
    //   break;
    case SIP::EID:
      SIP::handler(context);
      break;
    case DBCN::EID:
      DBCN::handler(context);
      break;
    default:
      ExceptionHandler::esr(context);
    }
    context->pc += 4;
  }
};

} // namespace QUARK::sbi
