// EPOS OStream Implementation

#include <main_traits.h>
#include <utility/ostream.h>
#include <architecture/cpu.h>

// Class Attributes
const char OStream::_digits[] = "0123456789abcdef";

// Class Methods
int OStream::itoa(int v, char * s)
{
    UInt32 i = 0;

    if(v < 0) {
        v = -v;
        s[i++] = '-';
    }

    return utoa(static_cast<UInt32>(v), s, i);
}

int OStream::utoa(unsigned int v, char * s, unsigned int i)
{
    // FIX: Guard against division-by-zero (_base == 0) or infinite loops (_base == 1)
    // Also ensures _base doesn't exceed the maximum standard digits array size (16)
    if (_base < 2 || _base > 16) {
        _base = 10;
    }

    UInt32 j;

    if(!v) {
        s[i++] = '0';
        return i;
    }

    if(v > 256) {
        if(_base == 8 || _base == 16)
            s[i++] = '0';
        if(_base == 16)
            s[i++] = 'x';
    }

    // This loop is now safe because _base is guaranteed to be >= 2
    for(j = v; j != 0; i++, j /= _base);
    for(j = 0; v != 0; j++, v /= _base)
        s[i - 1 - j] = _digits[v % _base];

    return i;
}


int OStream::llitoa(long long v, char * s)
{
    UInt32 i = 0;

    if(v < 0) {
        v = -v;
        s[i++] = '-';
    }

    return llutoa(static_cast<unsigned long long>(v), s, i);
}


int OStream::llutoa(unsigned long long v, char * s, unsigned int i)
{
    UInt64 j;

    if(!v) {
        s[i++] = '0';
        return i;
    }

    if(v > 256) {
        if(_base == 8 || _base == 16)
            s[i++] = '0';
        if(_base == 16)
            s[i++] = 'x';
    }

    for(j = v; j != 0; i++, j /= _base);
    for(j = 0; v != 0; j++, v /= _base)
        s[i - 1 - j] = _digits[v % _base];

    return i;
}


int OStream::ptoa(const void * p, char * s)
{
    CPU::Reg64 j, v = reinterpret_cast<CPU::Reg64>(p);

    s[0] = '0';
    s[1] = 'x';

    for(j = 0; j < sizeof(void *) * 2; j++, v >>= 4)
        s[2 + sizeof(void *) * 2 - 1 - j]
            = _digits[v & 0xf];

    return j + 2;
}

// SETUP does not handle global constructors, so kout and kerr must be
// manually initialized before use (at setup())
OStream kout, kerr;

//pthread_mutex_t OStream::MutexHandle;
//bool OStream::MutexInitialized = false;
