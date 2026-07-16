#pragma once

#include <architecture/riscv64/sbi/Base.hpp>
#include <architecture/riscv64/sbi/Counter.hpp>
#include <architecture/riscv64/sbi/FWFT.hpp>
#include <architecture/riscv64/sbi/Time.hpp>

namespace QUARK::sbi {

class Syscall {
  public:
    static constexpr unsigned int CODE = 9;
    static void dispatch(ContextFrame *c) {
        switch (c->a7) {
            case Base::EID: Base::handler(c); break;
            case Time::EID: Time::handler(c); break;
            case Counter::EID: Counter::handler(c); break;
            case FWFT::EID: FWFT::handler(c); break;
            default: ExceptionHandler::onTrap(c);
        }
        c->pc += 4;
    }
};

} // namespace QUARK::sbi
