#pragma once

// EPOS OStream Interface

#include <main_traits.h>
#include <system/types.h>
#include <utility/Console.hpp>

class OStream {
  public:
    struct Begl {};
    struct Endl {};
    struct Hex {};
    struct Dec {};
    struct Oct {};
    struct Bin {};
    struct Err {};

  private:
  public:
    OStream()
        : _base(10),
          _error(false) {
        // if (!MutexInitialized)
        //{
        //	pthread_mutexattr_t mutexAttr;
        //	pthread_mutexattr_init(&mutexAttr);
        //	pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
        //	pthread_mutex_init(&MutexHandle, &mutexAttr);
        //	pthread_mutexattr_destroy(&mutexAttr);
        //	MutexInitialized = true;
        // }
    }

    OStream &operator<<(const Begl &begl) {
        // if(Traits<System>::multicore)
        //     _print_preamble();
        return *this;
    }

    OStream &operator<<(const Endl &endl) {
        // if(Traits<System>::multicore)
        //     _print_trailler(_error);
        // print("\n");
		QUARK::Console::print('\n');
        _base = 10;
        return *this;
    }

    OStream &operator<<(const Hex &hex) {
        _base = 16;
        return *this;
    }
    OStream &operator<<(const Dec &dec) {
        _base = 10;
        return *this;
    }
    OStream &operator<<(const Oct &oct) {
        _base = 8;
        return *this;
    }
    OStream &operator<<(const Bin &bin) {
        _base = 2;
        return *this;
    }

    OStream &operator<<(const Err &err) {
        _error = true;
        return *this;
    }

    OStream &operator<<(char c) {
        char buf[2];
        buf[0] = c;
        buf[1] = '\0';
        print(buf);
        return *this;
    }
    OStream &operator<<(unsigned char c) { return operator<<(static_cast<UInt32>(c)); }

    OStream &operator<<(int i) {
        char buf[64];
        buf[itoa(i, buf)] = '\0';
        print(buf);
        return *this;
    }
    OStream &operator<<(short s) { return operator<<(static_cast<int>(s)); }
    OStream &operator<<(long l) { return operator<<(static_cast<long long>(l)); }

    OStream &operator<<(unsigned int u) {
        char buf[64];
        buf[utoa(u, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream &operator<<(unsigned short s) { return operator<<(static_cast<unsigned int>(s)); }

    OStream &operator<<(unsigned long l) { return operator<<(static_cast<unsigned long long int>(l)); }

    OStream &operator<<(long long u) {
        char buf[64];
        buf[llitoa(u, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream &operator<<(unsigned long long u) {
        char buf[64];
        buf[llutoa(u, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream &operator<<(const void *p) {
        char buf[64];
        buf[ptoa(p, buf)] = '\0';
        print(buf);
        return *this;
    }

    OStream &operator<<(const char *s) {
        print(s);
        return *this;
    }

    OStream &operator<<(float f) {
        if (f < 0.000000001f && f > -0.000000001f) {
            (*this) << "0.00000000";
            return *this;
        }
        if (f < 0) {
            (*this) << "-";
            f *= -1;
        }

        UInt64 b = (UInt64)f;
        (*this) << b << ".";

        int precision = 8;
        float diff    = (f - b) * 10;
        precision--;
        while (diff < 1 && precision > 0) {
            (*this) << "0";
            diff *= 10;
            precision--;
        }
        for (; precision > 0; precision--) {
            diff *= 10;
        }
        UInt64 m = (UInt64)(diff);
        (*this) << m;
        return *this;
    }

    OStream &operator<<(double d) {
        if (d < 0.000000001f && d > -0.000000001f) {
            (*this) << "0.000000000";
            return *this;
        }

        if (d < 0) {
            (*this) << "-";
            d *= -1;
        }

        UInt64 b = (UInt64)d;
        (*this) << b << ".";

        int precision = 9;
        double diff   = (d - b) * 10;
        precision--;
        while (diff < 1 && precision > 0) {
            (*this) << "0";
            diff *= 10;
            precision--;
        }
        for (; precision > 0; precision--) {
            diff *= 10;
        }
        UInt64 m = (UInt64)(diff);
        (*this) << m;
        return *this;
    }

  private:
    void print(const char *s) { QUARK::Console::print(s); }

    int itoa(int v, char *s);
    int utoa(unsigned int v, char *s, unsigned int i = 0);
    int llitoa(long long v, char *s);
    int llutoa(unsigned long long v, char *s, unsigned int i = 0);
    int ptoa(const void *p, char *s);

  private:
    int _base;
    volatile bool _error;

    static const char _digits[];
};

constexpr OStream::Begl begl;
constexpr OStream::Endl endl;
constexpr OStream::Hex hex;
constexpr OStream::Dec dec;
constexpr OStream::Oct oct;
constexpr OStream::Bin bin;

extern OStream kout, kerr;
