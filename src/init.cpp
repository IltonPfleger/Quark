#include <Thread.hpp>
#include <application/Application.hpp>
#include <architecture/Timer.hpp>
#include <machine/Machine.hpp>
#include <memory/Memory.hpp>
#include <utility/Deferred.hpp>

using namespace QUARK;

extern "C" void init() {
  Machine::init();

  if (CPU::id() == Traits<CPU>::BSP) {
    TraceIn();
    Application::reserve();
    Memory::init();
    Thread::init();
    Deferred::init();
    Application::init();
  }

  if constexpr (Traits<Timer>::Enable)
    Timer::init();

  if (CPU::id() == Traits<CPU>::BSP)
    TraceOut();

  CPU::barrier();
  CPU::mb();
  CPU::ib();

  Thread::run();
}
