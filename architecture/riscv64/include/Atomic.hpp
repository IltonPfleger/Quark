#pragma once

#include <Meta.hpp>

namespace QUARK::riscv64 {

class Atomic {
  public:
    template <typename T> static auto tsl(T &v) {
        typename Meta::Remove<T>::Result old;
        if constexpr (sizeof(T) == 4) {
            asm volatile("1: lr.w.aqrl  %0, (%1);"
                         "   li         t0, 1;"
                         "   sc.w.aqrl  t1, t0, (%1);"
                         "   bnez       t1, 1b;"
                         : "=&r"(old)
                         : "r"(&v)
                         : "t0", "t1", "memory");
        } else if constexpr (sizeof(T) == 8) {
            asm volatile("1: lr.d.aqrl  %0, (%1);"
                         "   li         t0, 1;"
                         "   sc.d.aqrl  t1, t0, (%1);"
                         "   bnez       t1, 1b;"
                         : "=&r"(old)
                         : "r"(&v)
                         : "t0", "t1", "memory");
        } else {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8);
        }

        return old;
    }

    template <typename T> static auto finc(T &v, typename Meta::Remove<T>::Result x = 1) {
        typename Meta::Remove<T>::Result old;
        if constexpr (sizeof(T) == 4) {
            asm volatile("1: lr.w.aqrl  %0, (%1);"
                         "   add        t0, %0, %2;"
                         "   sc.w.aqrl  t1, t0, (%1);"
                         "   bnez       t1, 1b;"
                         : "=&r"(old)
                         : "r"(&v), "r"(x)
                         : "t0", "t1", "memory");
        } else if constexpr (sizeof(T) == 8) {
            asm volatile("1: lr.d.aqrl  %0, (%1);"
                         "   add        t0, %0, %2;"
                         "   sc.d.aqrl  t1, t0, (%1);"
                         "   bnez       t1, 1b;"
                         : "=&r"(old)
                         : "r"(&v), "r"(x)
                         : "t0", "t1", "memory");
        } else {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8);
        }

        return old;
    }

    template <typename T> static bool cas(T &v, typename Meta::Remove<T>::Result expected, typename Meta::Remove<T>::Result desired) {
        typename Meta::Remove<T>::Result old;

        if constexpr (sizeof(T) == 4) {
            asm volatile("1: lr.w.aqrl  %0, (%1);"
                         "   bne        %0, %2, 2f;"
                         "   sc.w.aqrl  t0, %3, (%1);"
                         "   bnez       t0, 1b;"
                         "2:"
                         : "=&r"(old)
                         : "r"(&v), "r"(expected), "r"(desired)
                         : "t0", "memory");
        } else if constexpr (sizeof(T) == 8) {
            asm volatile("1: lr.d.aqrl  %0, (%1);"
                         "   bne        %0, %2, 2f;"
                         "   sc.d.aqrl  t0, %3, (%1);"
                         "   bnez       t0, 1b;"
                         "2:"
                         : "=&r"(old)
                         : "r"(&v), "r"(expected), "r"(desired)
                         : "t0", "memory");
        } else {
            static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Tamanho não suportado para CAS");
        }

        return old == expected;
    }

    template <typename T> static auto fdec(T &v, typename Meta::Remove<T>::Result x = 1) { return finc(v, -x); }

    template <typename T> static void store(T &v, typename Meta::Remove<T>::Result x) {
        asm volatile("" ::: "memory");
        *(volatile typename Meta::Remove<T>::Result *)&v = x;
    }

    template <typename T> static typename Meta::Remove<T>::Result load(const T &reference) {
        typename Meta::Remove<T>::Result response = *(volatile const typename Meta::Remove<T>::Result *)&reference;
        asm volatile("" ::: "memory");
        return response;
    }
};

} // namespace QUARK::riscv64
