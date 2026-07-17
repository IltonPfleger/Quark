#include <Semaphore.hpp>
#include <Thread.hpp>
#include <libraries/libc/string.h>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>
#include <utility/Debug.hpp>

using namespace QUARK;

static constexpr int Number     = 100;
static constexpr int Iterations = 1000;

size_t size(int &seed) {
    switch (seed++ % 31) {
        case 1: return 1;
        case 2: return 2;
        case 3: return 4;
        case 4: return 8;
        case 5: return 16;
        case 6: return 32;
        case 7: return 64;
        case 8: return 128;
        case 9: return 256;
        case 10: return 512;
        case 11: return 1024;
        case 12: return 2048;
        case 13: return 4096;
        case 14: return 8192;
        case 15: return 16384;
        case 16: return 32768;
        case 17: return 65536;
        case 18: return 4 - 1;
        case 19: return 8 - 1;
        case 20: return 16 - 1;
        case 21: return 32 - 1;
        case 22: return 64 - 1;
        case 23: return 128 - 1;
        case 24: return 256 - 1;
        case 25: return 512 - 1;
        case 26: return 1024 - 1;
        case 27: return 2048 - 1;
        case 28: return 4096 - 1;
        case 29: return 8192 - 1;
        case 30: return 16384 - 1;
        case 31: return 32768 - 1;
        case 32: return 65536 - 1;
    }
    return 0;
}

void *allocator(void *) {
    int iterations = Iterations;
    int seed       = 0;

    while (iterations--) {
        int s = size(seed);

        volatile unsigned char *buffer = new unsigned char[s];

        memset(const_cast<unsigned char *>(buffer), 0xAB, s);

        if (s > 0) {
            buffer[0]     = ~0;
            buffer[s - 1] = ~0;
        }

        delete[] buffer;
    }

    return 0;
}

int main(int, char *[]) {
    Thread *threads[Number];

    for (long i = 0; i < Number; i++) {
        threads[i] = new Thread(allocator, nullptr, Thread::Criterion(Thread::Criterion::NORMAL, i % Traits<CPU>::Active));
    }

    for (long i = 0; i < Number; i++) {
        delete threads[i];
    }
}
