#pragma once

namespace QUARK::sbi {

class StoreAddressMisaligned {
    using PageTable = MMU::PageTable;

  public:
    static constexpr uint32_t CODE = 6;

    static void dispatch(ContextFrame *context) {
        if ((context->status & MachineMode::PP) == MachineMode::PP_M) ExceptionHandler::esr(context);

        uintptr_t pc        = PageTable::virt2phys(context->pc);
        uint16_t compressed = Decoder::compressed(pc);

        uint8_t rs2  = 0;
        size_t width = 0;

        if (compressed) {
            uint8_t opcode = Decoder::opcode(compressed);
            uint8_t funct3 = Decoder::funct3(compressed);

            if (opcode == 0) {
                rs2 = 8 + ((compressed >> 2) & 0x7);

                if (funct3 == 6) {
                    width = 4;
                } else if (funct3 == 7) {
                    width = 8;
                } else {
                    ExceptionHandler::esr(context);
                    return;
                }
            } else if (opcode == 2) {
                rs2 = (compressed >> 2) & 0x1f;

                if (funct3 == 6) {
                    width = 4;
                } else if (funct3 == 7) {
                    width = 8;
                } else {
                    ExceptionHandler::esr(context);
                    return;
                }
            } else {
                ExceptionHandler::esr(context);
                return;
            }

            context->pc += 2;

        } else {
            uint32_t uncompressed = Decoder::uncompressed(pc);
            uint8_t funct3        = Decoder::funct3(uncompressed);

            rs2 = (uncompressed >> 20) & 0x1f;

            switch (funct3) {
                case 0x0: width = 1; break;
                case 0x1: width = 2; break;
                case 0x2: width = 4; break;
                case 0x3: width = 8; break;
                default: ExceptionHandler::esr(context); return;
            }

            context->pc += 4;
        }

        uintmax_t store   = (*context)[rs2];
        uintptr_t address = csrr<MachineMode::TVAL>();

        for (size_t i = 0; i < width; ++i) {
            uintptr_t target = PageTable::virt2phys(address + i);
            if (!VirtualCPU::sb(target, static_cast<uint8_t>((store >> (i * 8)) & 0xFF))) {
                ExceptionHandler::esr(context);
            }
        }
    }
};

}; // namespace QUARK::sbi
