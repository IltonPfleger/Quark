#pragma once

#include <architecture/ContextFrame.hpp>
#include <architecture/Decoder.hpp>
#include <architecture/ExceptionHandler.hpp>
#include <architecture/Modes.hpp>

namespace QUARK::sbi {

class StoreAccessFault {
  using PageTable = MMU::PageTable;

public:
  static constexpr uint32_t CODE = 7;

  static void dispatch(ContextFrame *context) {
    if ((context->status & MachineMode::PP) == MachineMode::PP_M) [[unlikely]]
      ExceptionHandler::esr(context);

    uintptr_t address = PageTable::virt2phys(csrr<MachineMode::TVAL>());
    uintptr_t pc = PageTable::virt2phys(context->pc);
    uint16_t compressed = Decoder::compressed(pc);
    uint8_t i;

    if (compressed) {
      i = Decoder::rs2(compressed);
      context->pc += 2;
    } else {
      uint32_t instruction = Decoder::uncompressed(pc);
      i = Decoder::rs2(instruction);
      context->pc += 4;
    }

    uintmax_t source = (*static_cast<const ContextFrame *>(context))[i];

    if (!VirtualCPU::write(address, source)) [[unlikely]]
      ExceptionHandler::esr(context);
  }
};

} // namespace QUARK::sbi
