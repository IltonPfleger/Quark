#pragma once

#include <Meta.hpp>

namespace QUARK {

typedef Meta::IF<sizeof(long) == 8, long, void>::Result int64_t;
typedef Meta::IF<sizeof(unsigned long) == 8, unsigned long, void>::Result uint64_t;
typedef Meta::IF<sizeof(void *) == 8, long, int>::Result intmax_t;
typedef Meta::IF<sizeof(void *) == 8, unsigned long, unsigned int>::Result uintmax_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef intmax_t intptr_t;
typedef uintmax_t uintptr_t;

typedef unsigned long size_t;

template <size_t Factor> class Duration {
    template <size_t> friend class Duration;

  public:
    constexpr Duration(uintmax_t value = 0)
        : value_(value) {}

    template <size_t U>
    constexpr Duration(const Duration<U> other)
        : value_((other.value_ * Factor) / U) {}

    template <size_t U> constexpr Duration operator+(const Duration<U> &other) const { return Duration(value_ + Duration(other).value_); }

    template <size_t U> constexpr Duration operator-(const Duration<U> &other) const { return Duration(value_ - Duration(other).value_); }

    constexpr operator uintmax_t() const { return value_; }

  private:
    uintmax_t value_;
};

using Second      = Duration<1>;
using Millisecond = Duration<1000>;
using Microsecond = Duration<1000000>;
using Nanosecond  = Duration<1000000000>;

typedef uintmax_t Hz;
typedef uintmax_t Tick;

} // namespace QUARK
