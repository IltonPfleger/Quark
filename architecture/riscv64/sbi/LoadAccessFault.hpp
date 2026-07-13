#pragma once

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/ExceptionHandler.hpp>
#include <architecture/riscv64/MMU.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>

namespace QUARK {

namespace sbi {

class LoadAccessFault {
    using PageTable = MMU::PageTable;

  public:
    static constexpr uint32_t CODE = 5;

    static void dispatch(ContextFrame *c) {
        if (((c->status >> 11) & 0x3) == 1) {

            uintptr_t address   = PageTable::virt2phys(csrr<MachineMode::TVAL>());
            uintptr_t pc        = PageTable::virt2phys(c->pc);
            uint16_t compressed = Decoder::compressed(pc);
            uint8_t i;
            if (compressed) {
                i = Decoder::rd(compressed);
                c->pc += 2;
            } else {
                uint32_t instruction = Decoder::uncompressed(pc);
                i                    = Decoder::rd(instruction);
                c->pc += 4;
            }

            if (VirtualCPU::read(address, reinterpret_cast<uint32_t *>(&(*c)[i]))) return;
        }
        ExceptionHandler::onTrap(c);
    };
};

} // namespace sbi

} // namespace QUARK
