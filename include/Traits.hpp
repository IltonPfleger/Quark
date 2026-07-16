#pragma once

#include <macros.hpp>
#include <types.hpp>

namespace QUARK {

class Thread;
class Machine;
class Timer;
class FixedCore;
class RR;
class Kernel;
class Payload;
class Debug;
class Alarm;
class Console;
class CPU;
class IPv4;
class Scheduler;
class Monitor;

template <typename T> struct Traits;

template <> struct Traits<Kernel> {
    static constexpr bool Multitask = false;
};

template <> struct Traits<Timer> {
    static constexpr Hz Frequency = 1'000;
    static constexpr bool Enable  = true;
};

template <> struct Traits<Alarm> {
    static constexpr Hz Frequency = Traits<Timer>::Frequency;
    static constexpr bool Enable  = true;
};

template <> struct Traits<Debug> {
    static constexpr bool Enable = true;
    static constexpr bool Error  = Enable && true;
    static constexpr bool Trace  = Enable && true;
};

template <> struct Traits<Scheduler> {
    typedef FixedCore Criterion;
};

} // namespace QUARK

#include <machine/Traits.hpp>

#include __PAYLOAD_TRAITS_HEADER

namespace QUARK {

template <> struct Traits<Thread> {
    static constexpr Hz Frequency           = Traits<Timer>::Frequency;
    static constexpr bool UserStack         = Traits<Payload>::Virtualized || Traits<Debug>::Error;
    static constexpr size_t KernelStackSize = Traits<Memory>::StackSize;
    static constexpr size_t UserStackSize   = UserStack ? Traits<Memory>::StackSize : 0;
};

} // namespace QUARK
