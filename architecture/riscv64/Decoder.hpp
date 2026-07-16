#pragma once

#include <libraries/libc/string.h>

namespace QUARK {

class Decoder {
  public:
    enum {
        SYSTEM = 0x73,
    };

    enum {
        CSRRS = 0x2,
    };

    enum {
        CYCLE   = 0xC00,
        TIME    = 0xC01,
        INSTRET = 0xC02,
    };

    [[nodiscard]] static constexpr bool fp(uint32_t instruction) {
        static constexpr uint8_t LD     = 0x07;
        static constexpr uint8_t SD     = 0x27;
        static constexpr uint8_t OP     = 0x53;
        static constexpr uint8_t FMADD  = 0x43;
        static constexpr uint8_t FMSUB  = 0x47;
        static constexpr uint8_t FNMSUB = 0x4B;
        static constexpr uint8_t FNMADD = 0x4F;
        uint8_t opcode                  = Decoder::opcode(instruction);
        return (opcode == LD) || (opcode == SD) || (opcode == OP) || (opcode == FMADD) || (opcode == FMSUB) || (opcode == FNMSUB) ||
               (opcode == FNMADD);
    }

    [[nodiscard]] static constexpr bool wfi(uint32_t instruction) {
        constexpr uint32_t encoding = 0x10500073;
        return instruction == encoding;
    }

    static uint8_t opcode(uint16_t instruction16) { return instruction16 & 0x3; }
    static uint8_t opcode(uint32_t instruction) { return instruction & 0x7F; }
    static uint8_t rd(uint16_t instruction16) { return 8 + ((instruction16 >> 2) & 0x7); }
    static uint8_t rd(uint32_t instruction) { return (instruction >> 7) & 0x1F; }
    static uint8_t rs2(uint16_t instruction16) { return 8 + ((instruction16 >> 2) & 0x7); }
    static uint8_t rs2(uint32_t instruction) { return (instruction >> 20) & 0x1f; }
    static uint8_t funct3(uint32_t instruction) { return (instruction >> 12) & 0x7; }
    static uint8_t funct3(uint16_t instruction16) { return (instruction16 >> 13) & 0x7; }
    static uint8_t rs1(uint32_t instruction) { return (instruction >> 15) & 0x1F; }
    static uint8_t rs1(uint16_t instruction16) { return 8 + ((instruction16 >> 7) & 0x7); }
    static uint16_t csr(uint32_t instruction) { return (instruction >> 20) & 0xFFF; }

    static uint32_t uncompressed(uintptr_t pc) {
        uint32_t result;
        memcpy(&result, reinterpret_cast<uint32_t *>(pc), sizeof(uint32_t));
        return result;
    }

    static uint16_t compressed(uintptr_t pc) {
        uint16_t result;
        memcpy(&result, reinterpret_cast<uint16_t *>(pc), sizeof(uint16_t));
        if ((result & 0x3) != 0x3) return result;
        return 0;
    }
};

} // namespace QUARK
