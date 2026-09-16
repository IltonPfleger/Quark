#pragma once

// EPOS Pseudo Random Number Generator Utility Declarations

// From http://en.wikipedia.org/wiki/Linear_congruential_generator
// with A = 2 and variable C on a simplification of:
// X1 = aX0 + c
// as X1 = (X0 << 1) xor C
#include <system/types.h>

class Random
{
private:
    static const UInt32 A = 1103515245;
    static const UInt32 C = 12345;
    static const UInt32 M = 4294967295U;

public:
    static int random() {
        _seed = ((_seed * A) + C) % M; //_seed = (_seed << 1) ^ n;
        return _seed;
    }

    static void seed(int value) {
        _seed = value;
    }

private:
    static int _seed;
};
