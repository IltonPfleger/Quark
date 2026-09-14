#pragma once

#include <architecture/sbi/Base.hpp>
#include <architecture/sbi/Counter.hpp>
#include <architecture/sbi/DBCN.hpp>
#include <architecture/sbi/FWFT.hpp>
#include <architecture/sbi/HSM.hpp>
#include <architecture/sbi/RFNC.hpp>
#include <architecture/sbi/SIP.hpp>
#include <architecture/sbi/Time.hpp>

namespace QUARK::sbi {

class Syscall {
public:
  static constexpr unsigned int CODE = 9;
  static void dispatch(ContextFrame *context) {

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
    case RFNC::EID:
      RFNC::handler(context);
      break;
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
