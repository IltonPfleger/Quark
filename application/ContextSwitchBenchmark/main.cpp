#include <Alarm.hpp>
#include <Semaphore.hpp>
#include <Thread.hpp>
#include <Traits.hpp>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>
#include <utility/Debug.hpp>
#include <utility/Delay.hpp>
#include <utility/Math.hpp>
#include <utility/collections/CircularBuffer.hpp>

using namespace QUARK;

static constexpr int Iterations = 1000;

using Unit   = Nanosecond;
using Buffer = collections::CircularBuffer<Unit, Iterations>;

Thread *threads[2];
Semaphore *a;
Semaphore *b;
Buffer aa;
Buffer bb;

void *first(void *) {
    new (&aa) Buffer();

    for (int i = 0; i < Iterations; i++) {
        a->v();
        b->p();
        CPU::IRQ::Guard _;
        Unit start = Timer::now();
        Thread::yield();
        Unit end = Timer::now();
        aa.insert(end - start);
    }

    return nullptr;
}

void *second(void *) {
    new (&bb) Buffer();

    for (int i = 0; i < Iterations; i++) {
        a->p();
        b->v();
        CPU::IRQ::Guard _;
        Unit start = Timer::now();
        Thread::yield();
        Unit end = Timer::now();
        bb.insert(end - start);
    }

    return nullptr;
}

int main(int, char *[]) {
    static_assert(Meta::Same<Traits<Scheduler>::Criterion, FixedCore>::Result);

    a = new Semaphore(0);
    b = new Semaphore(0);

    threads[0] = new Thread(first);
    threads[1] = new Thread(second);

    Thread::join(*threads[0]);
    Thread::join(*threads[1]);

    for (int i = 0; i < Iterations; i++) {
        aa[i] = aa[i] / 2;
        bb[i] = bb[i] / 2;
    }

    double mean = 0;
    for (int i = 0; i < Iterations; i++) {
        mean = mean + aa[i];
        mean = mean + bb[i];
    }

    mean = mean / (Iterations * 2);

    double variance = 0;
    for (int i = 0; i < Iterations; i++) {
        double da = aa[i] - mean;
        double db = bb[i] - mean;

        variance += da * da;
        variance += db * db;
    }

    variance /= (Iterations - 1);
    double std = Math::sqrt(variance);

    Console::println("Mean: ", mean);
    Console::println("Variance: ", variance);
    Console::println("Standard Deviation: ", std);

    delete threads[0];
    delete threads[1];
    delete a;
    delete b;
}
