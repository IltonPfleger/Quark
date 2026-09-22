#pragma once

// EPOS CPU Mediator Common Package

#include <utility/ostream.h>
#include <system/types.h>

class CPU
{
protected:
    static const bool _BIG_ENDIAN = (Traits<CPU>::ENDIANESS == Traits<CPU>::BIG);

protected:
    CPU() {}

public:
    typedef UInt8  Reg8;
    typedef UInt16 Reg16;
    typedef UInt32 Reg32;
    typedef UInt64 Reg64;
    typedef UInt   Reg;

    template <typename Reg>
    class Log_Address
    {
    public:
        Log_Address() {}
        Log_Address(const Log_Address & a) : _addr(a._addr) {}
        Log_Address(const Reg & a) : _addr(a) {}
        template<typename T>
        Log_Address(T * a) : _addr(Reg(a)) {}

        operator const Reg &() const { return _addr; }

        template<typename T>
        operator T *() const { return reinterpret_cast<T *>(_addr); }

        template<typename T>
        bool operator==(T a) const { return (_addr == Reg(a)); }
        template<typename T>
        bool operator< (T a) const { return (_addr < Reg(a)); }
        template<typename T>
        bool operator> (T a) const { return (_addr > Reg(a)); }
        template<typename T>
        bool operator>=(T a) const { return (_addr >= Reg(a)); }
        template<typename T>
        bool operator<=(T a) const { return (_addr <= Reg(a)); }

        template<typename T>
        Log_Address operator-(T a) const { return _addr - Reg(a); }
        template<typename T>
        Log_Address operator+(T a) const { return _addr + Reg(a); }
        template<typename T>
        Log_Address & operator+=(T a) { _addr += Reg(a); return *this; }
        template<typename T>
        Log_Address & operator-=(T a) { _addr -= Reg(a); return *this; }
        template<typename T>
        Log_Address & operator&=(T a) { _addr &= Reg(a); return *this; }
        template<typename T>
        Log_Address & operator|=(T a) { _addr |= Reg(a); return *this; }

        Log_Address & operator[](int i) { return *(this + i); }

        friend OStream & operator<<(OStream & os, const Log_Address & a) { os << reinterpret_cast<void *>(a._addr); return os; }

    private:
        Reg _addr;
    };

    template<typename Reg>
    using Phy_Address = Log_Address<Reg>;

    typedef UInt32 Hertz;

    class Context;

public:
    static UInt32 id();
    static UInt32 cores();

    static void halt() { for(;;); }

    static Hertz clock()  { return Traits<CPU>::CLOCK; }
    static void clock(const Hertz & frequency) {}
    static Hertz max_clock() { return Traits<CPU>::CLOCK; }
    static Hertz min_clock() { return Traits<CPU>::CLOCK; }

    static void fpu_save();
    static void fpu_restore();

    static bool tsl(volatile bool & lock) {
        bool old = lock;
        lock = 1;
        return old;
    }

    static int finc(volatile int & value) {
        int old = value;
        value++;
        return old;
    }

    static int fdec(volatile int & value) {
        int old = value;
        value--;
        return old;
    }

    static int cas(volatile int & value, int compare, int replacement) {
        int old = value;
        if(value == compare) {
            value = replacement;
        }
        return old;
    }

    template <int (* finc)(volatile int &)>
    static void smp_barrier(UInt32 cores, UInt32 id) {
        static volatile int ready[2];
        static volatile int i;

        int j = i;

        finc(ready[j]);
        if(id == 0) {
            while(ready[j] < int(cores));       // wait for all CPUs to be ready
            i = !i;                             // toggle ready
            ready[j] = 0;                       // signalizes waiting CPUs
        } else {
            while(ready[j]);                    // wait for CPU[0] signal
        }
    }

    static Reg64 _htole64(Reg64 v) { return (_BIG_ENDIAN) ? swap64(v) : v; }
    static Reg32 htole32(Reg32 v) { return (_BIG_ENDIAN) ? swap32(v) : v; }
    static Reg16 _htole16(Reg16 v) { return (_BIG_ENDIAN) ? swap16(v) : v; }
    static Reg64 letoh64(Reg64 v) { return _htole64(v); }
    static Reg32 letoh32(Reg32 v) { return htole32(v); }
    static Reg16 letoh16(Reg16 v) { return _htole16(v); }

    static Reg64 _htobe64(Reg64 v) { return (!_BIG_ENDIAN) ? swap64(v) : v; }
    static Reg32 htobe32(Reg32 v) { return (!_BIG_ENDIAN) ? swap32(v) : v; }
    static Reg16 _htobe16_(Reg16 v) { return (!_BIG_ENDIAN) ? swap16(v) : v; }
    static Reg64 betoh64(Reg64 v) { return _htobe64(v); }
    static Reg32 betoh32(Reg32 v) { return htobe32(v); }
    static Reg16 betoh16(Reg16 v) { return _htobe16_(v); }

    static Reg32 _htonl(Reg32 v) { return (_BIG_ENDIAN) ? v : swap32(v); }
    static Reg16 _htons(Reg16 v) { return (_BIG_ENDIAN) ? v : swap16(v); }
    static Reg32 _ntohl(Reg32 v) { return _htonl(v); }
    static Reg16 _ntohs(Reg16 v) { return _htons(v); }

    // static Reg64 _htole64(Reg64 v) { return v; }
    // static Reg32 htole32(Reg32 v) { return v; }
    // static Reg16 _htole16(Reg16 v) { return v; }
    // static Reg64 letoh64(Reg64 v) { return v; }
    // static Reg32 letoh32(Reg32 v) { return v; }
    // static Reg16 letoh16(Reg16 v) { return v; }

    // static Reg64 _htobe64(Reg64 v) { __asm__ __volatile__("bswap %0" : "=r"(v) : "0"(v), "r"(v)); return v; }
    // static Reg32 htobe32(Reg32 v) { __asm__ __volatile__("bswap %0" : "=r"(v) : "0"(v), "r"(v)); return v; }
    // static Reg16 _htobe16(Reg16 v) { return swap16(v); }
    // static Reg64 betoh64(Reg64 v) { return _htobe64(v); }
    // static Reg32 betoh32(Reg32 v) { return htobe32(v); }
    // static Reg16 betoh16(Reg16 v) { return _htobe16(v); }

    // static Reg32 _htonl(Reg32 v) { __asm__ __volatile__("bswap %0" : "=r"(v) : "0"(v), "r"(v)); return v; }
    // static Reg16 _htons(Reg16 v) { return swap16(v); }
    // static Reg32 _ntohl(Reg32 v) { return _htonl(v); }
    // static Reg16 _ntohs(Reg16 v) { return _htons(v); }

protected:
    static Reg64 swap64(Reg64 v) { return
        ((v & 0xff00000000000000ULL) >> 56) |
        ((v & 0x00ff000000000000ULL) >> 40) |
        ((v & 0x0000ff0000000000ULL) >> 24) |
        ((v & 0x000000ff00000000uLL) >> 8)  |
        ((v & 0x00000000ff000000ULL) << 8)  |
        ((v & 0x0000000000ff0000ULL) << 24) |
        ((v & 0x000000000000ff00ULL) << 40) |
        ((v & 0x00000000000000ffULL) << 56); }
    static Reg32 swap32(Reg32 v) { return
        ((v & 0xff000000) >> 24) |
        ((v & 0x00ff0000) >> 8) |
        ((v & 0x0000ff00) << 8) |
        ((v & 0x000000ff) << 24); }
    static Reg16 swap16(Reg16 v) { return
        ((v & 0xff00) >> 8) |
        ((v & 0x00ff) << 8); }
};

template<typename T>
inline T align32(const T & addr) { return (addr + 3) & ~3U; }
template<typename T>
inline T align64(const T & addr) { return (addr + 7) & ~7U; }
template<typename T>
inline T align128(const T & addr) { return (addr + 15) & ~15U; }

// #if defined(__CPU_H) && !defined(__cpu_common_only__)
// #include __CPU_H
// #endif
