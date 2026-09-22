#pragma once

#include <abi/ABI.hpp>
#include <types.hpp>

namespace QUARK {

template <typename Response = void *> class Syscall {
public:
  template <typename... Args> Syscall(Args &&...args) {
    static_assert(sizeof...(Args) <= 8);

    constexpr uintmax_t UNUSED = 0;

    uintmax_t argv[8] = {UNUSED, UNUSED, UNUSED, UNUSED,
                         UNUSED, UNUSED, UNUSED, UNUSED};

    int i = 0;
    ((argv[i++] = (uintmax_t)args), ...);

    register uintmax_t a0 asm("a0") = argv[0];
    register uintmax_t a1 asm("a1") = argv[1];
    register uintmax_t a2 asm("a2") = argv[2];
    register uintmax_t a3 asm("a3") = argv[3];
    register uintmax_t a4 asm("a4") = argv[4];
    register uintmax_t a5 asm("a5") = argv[5];
    register uintmax_t a6 asm("a6") = argv[6];
    register uintmax_t a7 asm("a7") = argv[7];

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                 : "memory");

    if constexpr (!Meta::IsVoid<Response>::Result) {
      response_ = (Response)a0;
    }
  }

  operator Response() const
    requires(!Meta::IsVoid<Response>::Result)
  {
    return response_;
  }

private:
  typename Meta::IF<!Meta::IsVoid<Response>::Result, Response,
                    Meta::Empty>::Result response_;
};

} // namespace QUARK
