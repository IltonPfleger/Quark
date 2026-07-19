#pragma once

namespace QUARK::sbi {

class StoreAddressMisaligned {
    using PageTable = MMU::PageTable;

  public:
    static constexpr uint32_t CODE = 6;

    static void dispatch(ContextFrame *c) {
        if (((c->status >> 11) & 0x3) != 1) ExceptionHandler::onTrap(c);

        uintptr_t pc        = PageTable::virt2phys(c->pc);
        uint16_t compressed = Decoder::compressed(pc);

        uint8_t rs2  = 0;
        size_t width = 0;

        if (compressed) {
            uint8_t opcode = Decoder::opcode(compressed);
            uint8_t funct3 = Decoder::funct3(compressed);

            if (opcode == 0) {
                rs2 = 8 + ((compressed >> 2) & 0x7);

                if (funct3 == 6) { // c.sw
                    width = 4;
                } else if (funct3 == 7) { // c.sd
                    width = 8;
                } else {
                    ExceptionHandler::onTrap(c);
                    return;
                }
            } else if (opcode == 2) {
                rs2 = (compressed >> 2) & 0x1f;

                if (funct3 == 6) {
                    width = 4;
                } else if (funct3 == 7) {
                    width = 8;
                } else {
                    ExceptionHandler::onTrap(c);
                    return;
                }
            } else {
                ExceptionHandler::onTrap(c);
                return;
            }

            c->pc += 2;

        } else {
            uint32_t uncompressed = Decoder::uncompressed(pc);
            uint8_t funct3        = Decoder::funct3(uncompressed);

            rs2 = (uncompressed >> 20) & 0x1f;

            switch (funct3) {
                case 0x0: // sb
                    width = 1;
                    break;
                case 0x1: // sh
                    width = 2;
                    break;
                case 0x2: // sw
                    width = 4;
                    break;
                case 0x3: // sd
                    width = 8;
                    break;
                default: ExceptionHandler::onTrap(c); return;
            }
            c->pc += 4;
        }

        uintmax_t store = (*c)[rs2];

        for (size_t i = 0; i < width; ++i) {
            uintptr_t address                     = PageTable::virt2phys(csrr<MachineMode::TVAL>() + i);
            *reinterpret_cast<uint8_t *>(address) = static_cast<uint8_t>((store >> (i * 8)) & 0xFF);
        }
    }
};

}; // namespace QUARK::sbi
