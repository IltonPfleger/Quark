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

  static void dispatch(ContextFrame *context) {
    if ((context->status & MachineMode::PP) == MachineMode::PP_M) [[unlikely]]
      ExceptionHandler::esr(context);

    uintptr_t address = PageTable::virt2phys(csrr<MachineMode::TVAL>());
    uintptr_t pc = PageTable::virt2phys(context->pc);
    uint16_t compressed = Decoder::compressed(pc);
    uint8_t i;

    if (compressed) {
      i = Decoder::rd(compressed);
      context->pc += 2;
    } else {
      uint32_t instruction = Decoder::uncompressed(pc);
      i = Decoder::rd(instruction);
      context->pc += 4;
    }

    uint32_t response = 0;
    if (!VirtualCPU::read(address, &response)) [[unlikely]]
      ExceptionHandler::esr(context);

    (*context)[i] = response;
  };
};

} // namespace sbi

} // namespace QUARK
