#pragma once

#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <scheduler/Scheduler.hpp>
#include <utility/Atomic.hpp>
#include <utility/collections/UnorderedList.hpp>

namespace QUARK {

class Thread {
  friend class PeriodicThread;
  friend class VirtualCPU;
  friend class Process;

public:
  typedef uintmax_t Flags;

  enum class State { RUNNING, READY, BLOCKED, FINISHING, FINISHED };
  enum : Flags { KERNEL, USER, DEAD };

  using Scheduler = QUARK::Scheduler;
  using Criterion = Scheduler::Criterion;
  using Node = Scheduler::Node;
  using List = collections::UnorderedList<Node, Spin>;

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
  List *blocking_;
  Atomic<Flags> flags_;

private:
  static constinit inline Scheduler scheduler_;
  static inline volatile unsigned int counter_;
  static inline Thread *volatile current_[Traits<CPU>::Active];
  static inline Thread *volatile previous_[Traits<CPU>::Active];
};

} // namespace QUARK
