#pragma once

#include <types.hpp>

namespace QUARK {

template <typename DEVICE> class Printer {
public:
  template <typename T> struct Hex {
    constexpr explicit Hex(T x) : value_(x) {}
    constexpr operator T() const { return value_; }

  private:
    const T value_;
  };

  template <typename T> Hex(T) -> Hex<T>;

  static void print(char character) {
    if (character == '\n')
      DEVICE::write('\r');
    DEVICE::write(character);
  }

  static void print(const char *s) {
    while (s && *s)
      print(*s++);
  }

  static void print(uintmax_t number) {
    char buffer[sizeof(number) * 8 / 3 + 1];
    int i = 0;

    if (number == 0) {
      print('0');
    } else {
      while (number != 0) {
        buffer[i++] = static_cast<char>((number % 10) + '0');
        number /= 10;
      }
      while (i > 0)
        print(buffer[--i]);
    }
  }

  static void print(intmax_t number) {
    if (number < 0) {
      print('-');
      number = -number;
    }
    print(static_cast<uintmax_t>(number));
  }

  static void print(Hex<uintmax_t> hex) {
    const char *digits = "0123456789abcdef";
    uintmax_t x = hex;
    char buffer[64];
    int i = 0;

    print("0x");

    if (x == 0) {
      print('0');
    } else {
      while (x > 0) {
        buffer[i++] = digits[x % 16];
        x = x / 16;
      }
      while (i > 0)
        print(buffer[--i]);
    }
  }

  static void print(double x) {
    if (x != x) {
      print("NaN");
      return;
    }

    if (x < 0) {
      print('-');
      x = -x;
    }

    if (x > 1e18) {
      print("Infinite");
      return;
    }

    print(static_cast<uintmax_t>(x));
    print('.');

    double decimals = (x - static_cast<double>(static_cast<uintmax_t>(x)));

    for (int i = 0; i < 10; i++) {
      decimals *= 10;
      int digit = static_cast<int>(decimals);
      print(static_cast<char>('0' + digit));
      decimals -= digit;
    }
  }

  static void print(bool boolean) { print(boolean ? "true" : "false"); }

  static void print(const void *pointer) { print(Hex(pointer)); }

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
};

} // namespace QUARK
