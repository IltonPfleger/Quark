#pragma once

namespace QUARK::sbi {

class LoadAddressMisaligned {
    using PageTable = MMU::PageTable;

  public:
    static constexpr uint32_t CODE = 4;

    static void dispatch(ContextFrame *c) {
        uintptr_t pc        = PageTable::virt2phys(c->pc);
        uint16_t compressed = Decoder::compressed(pc);

        uint8_t rd   = 0;
        size_t width = 0;
        bool sign    = true;

        if (compressed) {
            uint8_t opcode = Decoder::opcode(compressed);
            uint8_t funct3 = Decoder::funct3(compressed);

            if (opcode == 0) {
                rd = 8 + ((compressed >> 2) & 0x7);
                if (funct3 == 2) { // c.lw
                    width = 4;
                } else if (funct3 == 3) { // c.ld
                    width = 8;
                } else {
                    ExceptionHandler::onTrap(c);
                    return;
                }
            } else if (opcode == 2) {
                rd = (compressed >> 7) & 0x1f;
                if (funct3 == 2) { // c.lwsp
                    width = 4;
                } else if (funct3 == 3) { // c.ldsp
                    width = 8;
                } else {
                    ExceptionHandler::onTrap(c);
                    return;
                }
            } else {
                ExceptionHandler::onTrap(c);
                return;
            }

            sign = true;
            c->pc += 2;

        } else {
            uint32_t uncompressed = Decoder::uncompressed(pc);
            uint8_t funct3        = Decoder::funct3(uncompressed);
            rd                    = Decoder::rd(uncompressed);

            switch (funct3) {
                case 0x0:
                    width = 1;
                    sign  = true;
                    break;
                case 0x1:
                    width = 2;
                    sign  = true;
                    break;
                case 0x2:
                    width = 4;
                    sign  = true;
                    break;
                case 0x3:
                    width = 8;
                    sign  = true;
                    break;
                case 0x4:
                    width = 1;
                    sign  = false;
                    break;
                case 0x5:
                    width = 2;
                    sign  = false;
                    break;
                case 0x6:
                    width = 4;
                    sign  = false;
                    break;
                default: ExceptionHandler::onTrap(c); return;
            }
            c->pc += 4;
        }

        uintmax_t loaded = 0;
        for (size_t i = 0; i < width; ++i) {
            uintptr_t address = PageTable::virt2phys(csrr<MachineMode::TVAL>() + i);
            uint8_t data      = *reinterpret_cast<uint8_t *>(address);
            loaded |= (static_cast<uintmax_t>(data) << (i * 8));
        }

        if (sign && width < sizeof(uintmax_t)) {
            size_t shift = (sizeof(uintmax_t) - width) * 8;
            loaded       = static_cast<uintmax_t>((static_cast<intmax_t>(loaded << shift)) >> shift);
        }

        if (rd != 0) {
            (*c)[rd] = loaded;
        }
    }
};

}; // namespace QUARK::sbi
