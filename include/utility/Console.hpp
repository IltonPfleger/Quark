#pragma once

#include <Meta.hpp>
#include <Traits.hpp>

namespace QUARK {

class Console {
  using Device = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;

public:
  template <typename T> struct Hex {
    constexpr explicit Hex(T x) : value_(x) {}
    constexpr operator T() { return value_; }

  private:
    const T value_;
  };

  template <typename T> Hex(T) -> Hex<T>;

  static void panic();
  static void print(char);
  static void print(const char *);
  static void print(uintmax_t);
  static void print(intmax_t);
  static void print(Hex<uintmax_t>);
  static void print(double x);
  static void print(bool);

  static void print(const void *p) { print(Hex(p)); }

  template <typename T> static void print(Hex<T> hex) {
    print(Hex(reinterpret_cast<uintmax_t>(static_cast<T>(hex))));
  }

  template <Meta::Integer T> static void print(T x) {
    if constexpr (Meta::IsSigned<T>::Result) {
      print(static_cast<intmax_t>(x));
    } else {
      print(static_cast<uintmax_t>(x));
    }
  }

  template <typename First, typename Second, typename... Others>
  static void print(First &&first, Second &&second, Others &&...others) {
    print(first);
    print(second, others...);
  }

  template <typename First, typename... Others>
  static void println(First &&first, Others &&...others) {
    print(first);
    if constexpr (sizeof...(others) > 0) {
      print(others...);
    }
    print('\n');
  }

private:
  static bool panicked();

private:
  static volatile inline uintmax_t panic_ = 0;
};

} // namespace QUARK
