#pragma once

#include <architecture/ContextFrame.hpp>
#include <architecture/ExceptionHandler.hpp>
#include <architecture/MMU.hpp>
#include <architecture/Modes.hpp>
#include <architecture/VirtualCPU.hpp>

namespace QUARK {

namespace sbi {

class LoadAccessFault {
  using PageTable = MMU::PageTable;

public:
  static constexpr uint32_t CODE = 5;

  static void dispatch(ContextFrame *c) {
    if ((c->status & MachineMode::PP) == MachineMode::PP_M)
      ExceptionHandler::esr(c);

    uintptr_t address = PageTable::virt2phys(csrr<MachineMode::TVAL>());
    uintptr_t pc = PageTable::virt2phys(c->pc);
    uint16_t compressed = Decoder::compressed(pc);
    uint8_t i;
    if (compressed) {
      i = Decoder::rd(compressed);
    } else {
      uint32_t instruction = Decoder::uncompressed(pc);
      i = Decoder::rd(instruction);
    }

    if (VirtualCPU::read(address, reinterpret_cast<uint32_t *>(&(*c)[i]))) {
      if (compressed)
        c->pc += 2;
      else
        c->pc += 4;
      return;
    };

    ExceptionHandler::esr(c);
  };
};

} // namespace sbi

} // namespace QUARK
