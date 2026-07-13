#pragma once

#include <Meta.hpp>
#include <Traits.hpp>

namespace QUARK {

class Console {
    using Device = Meta::GetFromTypeList<Traits<UART>::Devices, 0>::Result;

  private:
    static bool panicked();

  public:
    static void panic();

    template <typename T> struct Hex {
        constexpr Hex(T x)
            : m_value(x) {}

        constexpr operator T() { return m_value; }

      private:
        T m_value;
    };

    template <typename T> Hex(T) -> Hex<T>;

    static void print(char);

    static void print(const char *s) {
        while (s && *s)
            print(*s++);
    }

    static void print(Hex<uintmax_t> x) {
        if (x == 0) {
            print("0x0");
            return;
        }

        char buffer[64];
        int i              = 0;
        const char *digits = "0123456789abcdef";

        while (x > 0) {
            buffer[i++] = digits[x % 16];
            x           = x / 16;
        }

        print("0x");
        while (i > 0)
            print(buffer[--i]);
    }

    static void print(uintmax_t x) {
        if (x == 0) {
            print('0');
            return;
        }

        char buffer[64];
        int i         = 0;
        uintmax_t val = static_cast<uintmax_t>(x);

        while (val > 0) {
            buffer[i++] = (val % 10) + '0';
            val /= 10;
        }

        while (i > 0)
            print(buffer[--i]);
    }

    static void print(intmax_t x) {
        if (x < 0) {
            print('-');
            x = -x;
        }
        print(static_cast<uintmax_t>(x));
    }

    static void print(const void *p) {
        if (p == nullptr) {
            print("0x0");
        } else {
            print(Hex(reinterpret_cast<uintptr_t>(p)));
        }
    }

    static void print(bool x) { print(x ? "true" : "false"); }

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
            print("INFINITE");
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

    template <Meta::Integer T> static void print(T x) {
        if constexpr (Meta::IsSigned<T>::Result) {
            print(static_cast<intmax_t>(x));
        } else {
            print(static_cast<uintmax_t>(x));
        }
    }
    template <Meta::Integer T> static void print(Hex<T> x) { print(Hex(static_cast<uintmax_t>(x))); }

    template <typename First, typename Second, typename... Others>
    static void print(First &&first, Second &&second, Others &&...others) {
        print(first);
        print(second, others...);
    }

    template <typename First, typename... Others> static void println(First &&first, Others &&...others) {
        print(first);
        if constexpr (sizeof...(others) > 0) {
            print(others...);
        }
        print('\n');
    }

  private:
    static volatile inline void *panic_ = 0;
};

} // namespace QUARK
