#include <Thread.hpp>
#include <architecture/Timer.hpp>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>

using namespace QUARK;

constexpr size_t CacheLineSize = 64;
constexpr size_t Size          = 8 * 1024 * 1024;
constexpr size_t Iterations    = 100;
alignas(CacheLineSize) uint8_t Buffers[Traits<CPU>::Active][Size];
Microsecond Durations[Traits<CPU>::Active];
Thread *Threads[Traits<CPU>::Active];

void *worker(void *argument) {
    size_t id = reinterpret_cast<size_t>(argument);

    volatile uint8_t *buffer = Buffers[id];

    Microsecond start = Timer::now();
    for (size_t it = 0; it < Iterations; ++it)
        for (size_t i = 0; i < Size; i += 64)
            buffer[i] = buffer[i] + 1;
    Microsecond end = Timer::now();

    Durations[id] = end - start;

    return nullptr;
}

int main(int, char *[]) {
    for (size_t i = 0; i < Traits<CPU>::Active; i++)
        Threads[i] = new Thread(worker, (void *)i, Thread::Criterion(Thread::Criterion::NORMAL, i));

    for (size_t i = 0; i < Traits<CPU>::Active; i++)
        Threads[i]->join();

    for (size_t i = 0; i < Traits<CPU>::Active; i++) {
        Console::println("Duration ", i, ": ", Durations[i], "us");
    }
}
