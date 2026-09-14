#pragma once

#include <architecture/ContextFrame.hpp>
#include <architecture/Decoder.hpp>
#include <architecture/ExceptionHandler.hpp>
#include <architecture/Modes.hpp>

namespace QUARK::sbi {

class StoreAccessFault {
  using PageTable = MMU::PageTable;

public:
  static constexpr unsigned int CODE = 7;

  static void dispatch(ContextFrame *c) {
    if ((c->status & MachineMode::PP) == MachineMode::PP_M)
      ExceptionHandler::esr(c);

    uintptr_t address = PageTable::virt2phys(csrr<MachineMode::TVAL>());
    uintptr_t pc = PageTable::virt2phys(c->pc);
    uint16_t compressed = Decoder::compressed(pc);
    uint8_t i;

    if (compressed) {
      i = Decoder::rs2(compressed);
      c->pc += 2;
    } else {
      uint32_t instruction = Decoder::uncompressed(pc);
      i = Decoder::rs2(instruction);
      c->pc += 4;
    }

    uintmax_t source = (*c)[i];

    if (VirtualCPU::write(address, source))
      return;

    ExceptionHandler::esr(c);
  }
};

} // namespace QUARK::sbi
