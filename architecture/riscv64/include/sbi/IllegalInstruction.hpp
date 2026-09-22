#pragma once

#include <Alarm.hpp>
#include <architecture/CLINT.hpp>
#include <architecture/ContextFrame.hpp>
#include <architecture/Decoder.hpp>
#include <architecture/ExceptionHandler.hpp>
#include <architecture/Modes.hpp>
#include <architecture/PMU.hpp>
#include <architecture/Timer.hpp>
#include <architecture/VirtualCPU.hpp>

namespace QUARK ::sbi {

class IllegalInstruction {
public:
  static constexpr unsigned int CODE = 2;

  static void dispatch(ContextFrame *context) {
    uint32_t instruction = context->value & 0xFFFFFFFF;
    if (Decoder::opcode(instruction) == Decoder::SYSTEM &&
        Decoder::funct3(instruction) == Decoder::CSRRS) {
      uint8_t rd = Decoder::rd(instruction);
      switch (Decoder::csr(instruction)) {
      case Decoder::TIME:
        (*context)[rd] = CLINT::mtime();
        break;
      case Decoder::CYCLE:
        (*context)[rd] = PMU::cycles();
        break;
      case Decoder::INSTRET:
        (*context)[rd] = PMU::instret();
        break;
      default:
        ExceptionHandler::esr(context);
      }
      context->pc += 4;
    } else if (Decoder::wfi(instruction)) {
      Thread::yield();
      context->pc += 4;
      //} else if (Decoder::floating(instruction) && !FPU::enabled(*context)) {
      //  FPU::enable<MachineMode>(context);
    } else {
      ExceptionHandler::esr(context);
    }
  }
};

} // namespace QUARK::sbi
