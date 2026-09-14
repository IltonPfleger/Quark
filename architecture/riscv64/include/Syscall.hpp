#pragma once

#include <Meta.hpp>
#include <abi/ABI.hpp>

namespace QUARK {

template <typename Response = void *> class Syscall {
public:
  template <typename... Args> Syscall(ABI::Function call, Args &&...args) {
    static_assert(sizeof...(Args) <= 6);

    constexpr long UNUSED = 0;
    long argv[6] = {UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED};

    if constexpr (sizeof...(Args) > 0) {
      int i = 0;
      ((argv[i++] = (long)args), ...);
    }

    register long a0 asm("a0") = argv[0];
    register long a1 asm("a1") = argv[1];
    register long a2 asm("a2") = argv[2];
    register long a3 asm("a3") = argv[3];
    register long a4 asm("a4") = argv[4];
    register long a5 asm("a5") = argv[5];

    register long a7 asm("a7") = static_cast<long>(call);

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7)
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
