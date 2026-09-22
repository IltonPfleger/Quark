#pragma once

// EPOS AES Mediator Common Package

class AES_Common
{
public:
    enum Mode {
        ECB,
        CBC,
    };

protected:
    AES_Common() {}

public:
    const Mode & mode();

    void encrypt(const unsigned char * data, const unsigned char * key, unsigned char * result);
    void decrypt(const unsigned char * data, const unsigned char * key, unsigned char * result);
};


#if !defined(__AES_H) && !defined(__aes_common_only__)
// If the machine does not feature an AES accelerator, then use a software lib
#include <utility/aes.h>
template<UInt32 KEY_SIZE>
using AES = SWAES<KEY_SIZE>;
#endif

#if defined(__AES_H) && !defined(__aes_common_only__)
#include __AES_H
template<UInt32 KEY_SIZE>
using AES = HWAES<KEY_SIZE>;
#endif
