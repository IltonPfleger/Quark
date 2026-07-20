#include <Payload.hpp>
#include <Thread.hpp>
#include <architecture/Timer.hpp>
#include <machine/Machine.hpp>
#include <memory/Memory.hpp>
#include <utility/WorkerManager.hpp>

using namespace QUARK;

extern "C" void init() {
    if (CPU::id() == Traits<CPU>::BSP) {
        Console::println('\n');
        TraceIn();
        Payload::alloc();
        Memory::init();
        Thread::init();
        Payload::init();
        WorkerManager::init();
    }

    if constexpr (Traits<Timer>::Enable) Timer::init();

    CPU::barrier();

    Thread::run();
}

extern "C" __attribute__((optimize("O0"), naked, used, section(".init"))) void _init() {
    CPU::init();
    Machine::init();
    init();
}
