#include <Semaphore.hpp>
#include <Thread.hpp>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>
#include <utility/Delay.hpp>

using namespace QUARK;

static constexpr int Number     = 10;
static constexpr int Iterations = 10000;

Semaphore *console;
Semaphore *start;
Thread *threads[Number];

void *pi(void *) {
    start->p();

    const double step = 1.0 / Iterations;

    double sum = 0.0;

    for (int i = 0; i < Iterations; i++) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }

    double pi = sum * step;

    console->p();
    Console::println("<", CPU::id(), ">", pi);
    console->v();

    return nullptr;
}

int main(int, char *[]) {
    console = new Semaphore(1);
    start   = new Semaphore(0);

    for (int i = 0; i < Number; i++)
        threads[i] = new Thread(pi);

    for (int i = 0; i < Number; i++)
        start->v();

    for (int i = 0; i < Number; i++)
        threads[i]->join();

    for (int i = 0; i < Number; i++)
        delete threads[i];

    delete console;
    delete start;

    return 0;
}
