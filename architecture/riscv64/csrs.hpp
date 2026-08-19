#pragma once

#include <architecture/riscv64/Modes.hpp>
#include <types.hpp>

namespace QUARK {

template <int R> static inline uintmax_t csrrw(auto v) {
  uintmax_t old;
  asm volatile("csrrw %0, %1, %2" : "=r"(old) : "i"(R), "r"(v) : "memory");
  return old;
}
template <int R> static inline auto csrrc(auto c) {
  uintmax_t r;
  asm volatile("csrrc %0, %1, %2" : "=r"(r) : "i"(R), "r"(c));
  return r;
}

template <int R> static inline void csrw(auto r) {
  asm volatile("csrw %0, %1" ::"i"(R), "r"(r));
}

template <int R> static inline auto csrr() {
  uintmax_t r;
  asm volatile("csrr %0, %1" : "=r"(r) : "i"(R));
  return r;
}

template <int R> static inline void csrs(auto r) {
  asm volatile("csrs %0, %1" ::"i"(R), "r"(r));
}

template <int R> static inline void csrc(auto r) {
  asm volatile("csrc %0, %1" ::"i"(R), "r"(r));
}

static inline void fcsr(auto r) { asm("csrw fcsr, %0" ::"r"(r)); }

static inline size_t mhartid() { return csrr<MachineMode::HARTID>(); }

} // namespace QUARK
