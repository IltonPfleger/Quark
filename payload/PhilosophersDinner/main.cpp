#include <Semaphore.hpp>
#include <Thread.hpp>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>
#include <utility/Debug.hpp>

using namespace QUARK;

static constexpr int Number     = 1000;
static constexpr int Iterations = 100;

Semaphore *forks[Number];
Semaphore *console;
Semaphore *finish;
Thread *threads[Number];

void *philosopher(void *p) {
    unsigned int id = (unsigned int)(unsigned long)p;

    int iterations = Iterations;
    int left       = id;
    int right      = (id + 1) % Number;

    while (iterations--) {
        console->p();
        Console::println("<", CPU::id(), ">", " Filósofo ", id, " está pensando! <", iterations, ">");
        console->v();

        if (id == Number - 1) {
            forks[right]->p();
            forks[left]->p();
        } else {
            forks[left]->p();
            forks[right]->p();
        }

        console->p();
        Console::println("<", CPU::id(), ">", " Filósofo ", id, " está comendo! <", iterations, ">");
        console->v();

        forks[right]->v();
        forks[left]->v();
    }

    return 0;
}

int main(int, char *[]) {
    console = new Semaphore(0);
    finish  = new Semaphore(0);

    for (long i = 0; i < Number; i++)
        forks[i] = new Semaphore(1);

    for (long i = 0; i < Number; i++)
        threads[i] = new Thread(philosopher, (void *)i, Thread::Criterion::NORMAL);

    console->v();

    for (long i = 0; i < Number; i++)
        threads[i]->join();
}
