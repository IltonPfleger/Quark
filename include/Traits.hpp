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
    static constexpr Hz Frequency = 100;
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

template <> struct Traits<Console> {
    static constexpr size_t Columns = 120;
};

template <> struct Traits<Scheduler> {
    typedef FixedCore Criterion;
};

} // namespace QUARK

#include <machine/Traits.hpp>

#include __PAYLOAD_TRAITS_HEADER

namespace QUARK {

template <> struct Traits<Thread> {
    static constexpr Hz Frequency             = Traits<Timer>::Frequency;
    static constexpr bool IsolatedKernelStack = Traits<Payload>::Virtualized || Traits<Debug>::Error;
    static constexpr size_t UserStackSize     = Traits<Memory>::StackSize;
    static constexpr size_t KernelStackSize   = IsolatedKernelStack ? Traits<Memory>::StackSize : 0;
};

} // namespace QUARK
