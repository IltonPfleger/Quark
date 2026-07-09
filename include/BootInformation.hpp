#pragma once

#include <memory/Chunk.hpp>
#include <types.hpp>

namespace QUARK {

namespace BootInformation {

extern "C" {
extern uint8_t __kernel_start[];
extern uint8_t __kernel_end[];

extern uint8_t __init_start[];
extern uint8_t __init_end[];

extern uint8_t __text_start[];
extern uint8_t __text_end[];

extern uint8_t __rodata_start[];
extern uint8_t __rodata_end[];

extern uint8_t __data_start[];
extern uint8_t __data_end[];

extern uint8_t __bss_start[];
extern uint8_t __bss_end[];
}

inline Chunk make(uint8_t *start, uint8_t *end) {
    uintptr_t s = reinterpret_cast<uintptr_t>(start);
    uintptr_t e = reinterpret_cast<uintptr_t>(end);
    return Chunk(s, e - s);
}

inline Chunk text() { return make(__text_start, __text_end); }
inline Chunk data() { return make(__data_start, __data_end); }
inline Chunk rodata() { return make(__rodata_start, __rodata_end); }
inline Chunk bss() { return make(__bss_start, __bss_end); }
inline Chunk init() { return make(__init_start, __init_end); }
inline Chunk kernel() { return make(__kernel_start, __kernel_end); }

} // namespace BootInformation

__attribute__((section(".__payload_mm__"))) inline Chunk __pmm;
__attribute__((section(".__all_mm__"))) inline Chunk __amm;
__attribute__((section(".__boot_mm__"))) inline Chunk __bmm;

} // namespace QUARK
