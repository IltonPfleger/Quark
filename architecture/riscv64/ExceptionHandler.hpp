#pragma once

#include <architecture/riscv64/CPU.hpp>
#include <architecture/riscv64/csrs.hpp>
#include <utility/Console.hpp>

namespace QUARK {

class ExceptionHandler {
  public:
    static void esr(ContextFrame *context) {
        Console::panic();
        Console::print('\n');
        Console::println("Ohh, It's a Trap! <", CPU::id(), ">");
        Console::println("context: ", context);
        Console::println("pc: ", Console::Hex(context->pc));
        Console::println("cause: ", Console::Hex(context->cause));
        Console::println("status: ", Console::Hex(context->status));
        Console::println("tval: ", Console::Hex(context->value));
        CPU::halt();
    }
};

} // namespace QUARK
