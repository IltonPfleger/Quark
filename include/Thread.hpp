#pragma once

#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <scheduler/Scheduler.hpp>

namespace QUARK {

class Thread {
  friend class PeriodicThread;
  friend class VirtualCPU;
  friend class Process;

public:
  typedef uintmax_t Flags;

  enum class State { RUNNING, READY, WAITING, FINISHING, FINISHED };
  enum : Flags { KERNEL = 0, USER = 1 << 0, SUCCESS = 1 << 1, ERROR = 1 << 2 };

  using Scheduler = QUARK::Scheduler;
  using Criterion = Scheduler::Criterion;
  using Node = Scheduler::Node;
  using List = collections::FIFO<Node, Spin>;

  using Return = void *;
  using Argument = void *;
  using Function = Return (*)(Argument);
  using Context = CPU::Context;

  Thread(const Thread &) = delete;
  Thread(const Thread &&) = delete;
  Thread &operator=(Thread &&) = delete;
  Thread &operator=(const Thread &) = delete;
  Thread(Function, Argument = 0, Criterion = Criterion::NORMAL, Flags = USER,
         Process * = nullptr);
  ~Thread();

  static void init();
  static Thread *running();
  static void run();
  static void sleep(List *);
  static bool wakeup(List *);
  static void yield();
  static void reschedule();
  static void onTick();
  static void exit();
  void join();
  void kill();

private:
  static void entry(Function, Argument);
  static void dispatch(Thread *, Thread *);
  static Return idle(Argument);
  static void epilogue();
  static Thread *next(decltype(Criterion::IDLE) = Criterion::IDLE);

private:
  Process *process_;
  void *stack_;
  void *kstack_;
  Node node_;
  volatile State state_;
  Context context_;
  Flags flags_;

private:
  static constinit inline Scheduler scheduler_;
  static inline volatile unsigned int s_count;
  static inline Thread *volatile previous_[Traits<CPU>::Active];
  static inline List *volatile blocking_[Traits<CPU>::Active];
};

} // namespace QUARK
