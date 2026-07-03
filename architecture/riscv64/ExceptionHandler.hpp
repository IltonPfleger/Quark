#pragma once

#include <architecture/riscv64/CPU.hpp>
#include <architecture/riscv64/csrs.hpp>
#include <utility/Console.hpp>

namespace QUARK {

class ExceptionHandler {
  public:
    static void onTrap(ContextFrame *c) {
        Console::panic();
        Console::println("\n<", CPU::id(), "> Ohh, It's a Trap!");
        Console::println("context: ", c);
        Console::println("pc: ", Console::Hex(c->pc));
        Console::println("cause: ", Console::Hex(c->cause));
        Console::println("status: ", Console::Hex(c->status));
        Console::println("tval: ", Console::Hex(c->value));
        CPU::halt();
    }
};

} // namespace QUARK
