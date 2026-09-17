#pragma once

#include <types.hpp>

namespace QUARK {

class Application;
class Thread;
class Machine;
class Timer;
class FixedCore;
class RR;
class Kernel;
class Application;
class Debug;
class Alarm;
class Console;
class CPU;
class IPv4;
class Scheduler;
class Monitor;
class Deferred;
class Process;
class VirtualMachine;

template <typename T> struct Traits;

template <> struct Traits<Kernel> {
  enum { LIBRARY, KERNEL };
  static constexpr int Mode = LIBRARY;
};

template <> struct Traits<Timer> {
  static constexpr Hz Frequency = 500;
  static constexpr bool Enable = true;
};

template <> struct Traits<Alarm> {
  static constexpr Hz Frequency = Traits<Timer>::Frequency;
  static constexpr bool Enable = true;
};

template <> struct Traits<Debug> {
  static constexpr bool Enable = true;
  static constexpr bool Error = Enable && true;
  static constexpr bool Trace = Enable && true;
};

template <> struct Traits<Scheduler> {
  typedef FixedCore Criterion;
};

} // namespace QUARK

#include <application/Traits.hpp>
#include <machine/Traits.hpp>

namespace QUARK {

template <> struct Traits<Thread> {
  static constexpr Hz Frequency = Traits<Timer>::Frequency;
  static constexpr bool UserStack = Traits<Application>::Virtualization;
  static constexpr size_t KernelStackSize = Traits<Memory>::StackSize;
  static constexpr size_t UserStackSize = UserStack ? KernelStackSize : 0;
};

} // namespace QUARK
