#pragma once
#include <Alarm.hpp>
#include <CPU.hpp>
#include <Machine.hpp>
#include <Memory.hpp>
#include <Traits.hpp>

template <bool enabled = Traits::Timer::Enable>
class _Timer {
   public:
    static void init() {}
    static void handler() {}
};

template <>
class _Timer<true> {
    struct Channel {
        enum { SCHEDULER, ALARM, COUNT };
        Tick _initial;
        Tick _current[Machine::CPUS];
        void (*_handler)();
    };

    static inline Channel _channels[Channel::COUNT];

    static void reset() {
        *reinterpret_cast<volatile uintmax_t *>(Machine::CLINT::MTIMECMP + CPU::core() * 8) =
            *Machine::CLINT::MTIME + (Machine::CLINT::CLOCK / Traits::Timer::Frequency);
    }

   public:
    static void init() {
        CPU::Interrupt::Timer::enable();

        if constexpr (Traits::Scheduler<Thread>::Criterion::Timed) {
            _channels[Channel::SCHEDULER]._handler = Thread::reschedule;
            _channels[Channel::SCHEDULER]._initial = Traits::Timer::Frequency / Traits::Scheduler<Thread>::Frequency;
            _channels[Channel::SCHEDULER]._current[CPU::core()] = _channels[Channel::SCHEDULER]._initial;
        }

        if constexpr (Traits::Alarm::Enable) {
            _channels[Channel::ALARM]._handler              = Alarm::handler;
            _channels[Channel::ALARM]._initial              = Traits::Timer::Frequency / Traits::Alarm::Frequency;
            _channels[Channel::ALARM]._current[CPU::core()] = _channels[Channel::ALARM]._initial;
        }
    }

    static void handler() {
        reset();
        // if constexpr (Traits::Alarm::Enable) {
        //     if (--_channels[Channel::ALARM]._current[CPU::core()] == 0) {
        //         _channels[Channel::ALARM]._current[CPU::core()] = _channels[Channel::ALARM]._initial;
        //         _channels[Channel::ALARM]._handler();
        //     }
        // }

        if constexpr (Traits::Scheduler<Thread>::Criterion::Timed) {
            if (--_channels[Channel::SCHEDULER]._current[CPU::core()] == 0) {
                _channels[Channel::SCHEDULER]._current[CPU::core()] = _channels[Channel::SCHEDULER]._initial;
                _channels[Channel::SCHEDULER]._handler();
            }
        }
    }
};

using Timer = _Timer<>;
