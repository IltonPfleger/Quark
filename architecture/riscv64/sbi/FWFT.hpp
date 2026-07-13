#pragma once

#include <architecture/riscv64/ContextFrame.hpp>

namespace QUARK::sbi {

class FWFT {
  public:
    static constexpr unsigned int EID = 0x46574654;

    static void handler(ContextFrame *c) {
        switch (c->a6) {
            default:
                break;
                // Console::println("FWFT: ", Console::Hex(c->a6));
                // Console::println(Console::Hex(c->a0));
                // Console::println(Console::Hex(c->a1));
                // Console::println(Console::Hex(c->a2));
        }
        c->a0 = 0;
    }
};

} // namespace QUARK::sbi
