#pragma once

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/Decoder.hpp>
#include <architecture/riscv64/ExceptionHandler.hpp>
#include <architecture/riscv64/Modes.hpp>

namespace QUARK::sbi {

class StoreAccessFault {
    using PageTable = MMU::PageTable;

  public:
    static constexpr unsigned int CODE = 7;

    static void dispatch(ContextFrame *c) {
        if (((c->status >> 11) & 0x3) != 1) ExceptionHandler::onTrap(c);

        uintptr_t address   = PageTable::virt2phys(csrr<MachineMode::TVAL>());
        uintptr_t pc        = PageTable::virt2phys(c->pc);
        uint16_t compressed = Decoder::compressed(pc);
        uint8_t i;
        if (compressed) {
            i = Decoder::rs2(compressed);
            c->pc += 2;
        } else {
            uint32_t instruction = Decoder::uncompressed(pc);
            i                    = Decoder::rs2(instruction);
            c->pc += 4;
        }

        uintmax_t source = (*c)[i];
        if (VirtualCPU::write(address, source)) return;

        ExceptionHandler::onTrap(c);
    }
};

} // namespace QUARK::sbi
