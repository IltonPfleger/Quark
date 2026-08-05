#include <Thread.hpp>
#include <architecture/Timer.hpp>
#include <machine/Machine.hpp>
#include <memory/Memory.hpp>
#include <payload/Payload.hpp>
#include <utility/Deferred.hpp>

using namespace QUARK;

extern "C" void init() {
    if (CPU::id() == Traits<CPU>::BSP) {
        TraceIn();
        Machine::init();
        Payload::alloc();
        Memory::init();
        Thread::init();
        Payload::init();
        Deferred::init();
    }

    if constexpr (Traits<Timer>::Enable) Timer::init();

    CPU::barrier();

    Thread::run();
}
