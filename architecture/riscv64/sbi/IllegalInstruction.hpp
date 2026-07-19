#pragma once

#include <Alarm.hpp>
#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/Decoder.hpp>
#include <architecture/riscv64/ExceptionHandler.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/PMU.hpp>
#include <architecture/riscv64/Timer.hpp>

namespace QUARK ::sbi {

class IllegalInstruction {
  public:
    static constexpr unsigned int CODE = 2;

    static void dispatch(ContextFrame *context) {
        uint32_t instruction = context->value & 0xFFFFFFFF;
        if (Decoder::opcode(instruction) == Decoder::SYSTEM && Decoder::funct3(instruction) == Decoder::CSRRS) {
            uint8_t rd = Decoder::rd(instruction);
            switch (Decoder::csr(instruction)) {
                case Decoder::TIME: (*context)[rd] = CLINT::mtime(); break;
                case Decoder::CYCLE: (*context)[rd] = PMU::cycles(); break;
                case Decoder::INSTRET: (*context)[rd] = PMU::instret(); break;
                default: ExceptionHandler::onTrap(context);
            }
            context->pc += 4;
        } else if (Decoder::wfi(instruction)) {
            Alarm(0);
            context->pc += 4;
        } else if (Decoder::fp(instruction) && !FPU::enabled(context)) {
            FPU::enable<MachineMode>(context);
        } else {
            ExceptionHandler::onTrap(context);
        }
    }
};

} // namespace QUARK::sbi
