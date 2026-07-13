#ifndef __QUARK_RISCV64_SBI_DECODER__
#define __QUARK_RISCV64_SBI_DECODER__

namespace QUARK {

class Decoder {
  public:
    [[nodiscard]] static constexpr bool fp(uint32_t instruction) {
        static constexpr uint8_t LD     = 0x07;
        static constexpr uint8_t SD     = 0x27;
        static constexpr uint8_t OP     = 0x53;
        static constexpr uint8_t FMADD  = 0x43;
        static constexpr uint8_t FMSUB  = 0x47;
        static constexpr uint8_t FNMSUB = 0x4B;
        static constexpr uint8_t FNMADD = 0x4F;
        uint8_t opcode                  = Decoder::opcode(instruction);
        return (opcode == LD) || (opcode == SD) || (opcode == OP) || (opcode == FMADD) || (opcode == FMSUB) ||
               (opcode == FNMSUB) || (opcode == FNMADD);
    }

    [[nodiscard]] static constexpr bool wfi(uint32_t instruction) {
        constexpr uint32_t encoding = 0x10500073;
        return instruction == encoding;
    }

    [[nodiscard]]
    static bool rdtime(uint32_t instruction) {
        static constexpr uint32_t value = 0xC0102073;
        static constexpr uint32_t mask  = 0xFFF0707F;
        return (instruction & mask) == value;
    }

    [[nodiscard]]
    static uint8_t opcode(uintptr_t instruction) {
        if ((instruction & 0x3) != 0x3) return instruction & 0x3;
        return instruction & 0x7F;
    }

    static uint8_t rd(uint16_t instruction16) { return 8 + ((instruction16 >> 2) & 0x7); }
    static uint8_t rd(uint32_t instruction) { return (instruction >> 7) & 0x1F; }
    static uint8_t rs2(uint16_t instruction) { return 8 + ((instruction >> 2) & 0x7); }
    static uint8_t rs2(uint32_t instruction) { return (instruction >> 20) & 0x1f; }

    static uint32_t uncompressed(uintptr_t pc) { return *reinterpret_cast<uint32_t *>(pc); }
    static uint16_t compressed(uintptr_t pc) {
        const uint16_t instruction16 = *reinterpret_cast<const uint16_t *>(pc);
        if ((instruction16 & 0x3) != 0x3) return instruction16;
        return 0;
    }
};

} // namespace QUARK

#endif
