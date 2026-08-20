#pragma once

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/sbi/DBCN.hpp>
#include <architecture/riscv64/sbi/HSM.hpp>
#include <architecture/riscv64/sbi/RFNC.hpp>
#include <architecture/riscv64/sbi/SIP.hpp>
#include <architecture/riscv64/sbi/Time.hpp>

namespace QUARK::sbi {

class Base {
public:
  static constexpr unsigned int EID = 0x10;

  enum {
    GET_SPEC_VERSION = 0,
    GET_IMPLEMENTATION_ID = 1,
    GET_IMPLEMENTATION_VERSION = 2,
    PROBE_EXTENSION = 3,
    GET_MVENDORID = 4,
    GET_MARCHID = 5,
    GET_MIMPID = 6,
  };

  static void handler(ContextFrame *c) {
    switch (c->a6) {
    case GET_MVENDORID: {
      c->a0 = 0;
      c->a1 = csrr<MachineMode::VENDORID>();
      break;
    }
    case GET_MARCHID: {
      c->a0 = 0;
      c->a1 = csrr<MachineMode::ARCHID>();
      break;
    }
    case GET_MIMPID: {
      c->a0 = 0;
      c->a1 = csrr<MachineMode::IMPID>();
      break;
    }
    case GET_SPEC_VERSION: {
      c->a0 = 0;
      c->a1 = (2 << 24) | 0;
      break;
    }
    case GET_IMPLEMENTATION_ID: {
      c->a0 = 0;
      c->a1 = 1;
      break;
    }
    case GET_IMPLEMENTATION_VERSION: {
      c->a0 = 0;
      c->a1 = 0;
      break;
    }
    case PROBE_EXTENSION: {
      if (c->a0 == Base::EID)
        c->a1 = 1;
      else if (c->a0 == Time::EID)
        c->a1 = 1;
      else if (c->a0 == HSM::EID)
        c->a1 = 1;
      else if (c->a0 == RFNC::EID)
        c->a1 = 0;
      else if (c->a0 == DBCN::EID)
        c->a1 = 1;
      else if (c->a0 == SIP::EID)
        c->a1 = 1;
      else
        c->a1 = 0;

      c->a0 = 0;

      break;
    }
    }
  }
};

} // namespace QUARK::sbi
