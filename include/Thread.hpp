#pragma once

#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <scheduler/Scheduler.hpp>

namespace QUARK {

class Thread {
    friend class PeriodicThread;

  public:
    enum class State { RUNNING, READY, WAITING, FINISHING, FINISHED };
    enum class Domain { USER, KERNEL };

    using Scheduler = QUARK::Scheduler;
    using Criterion = Scheduler::Criterion;
    using Node      = Scheduler::Node;
    using List      = collections::FIFO<Node>;

    using Return   = void *;
    using Argument = void *;
    using Function = Return (*)(Argument);
    using Context  = CPU::Context;

    Thread(const Thread &)            = delete;
    Thread(const Thread &&)           = delete;
    Thread &operator=(Thread &&)      = delete;
    Thread &operator=(const Thread &) = delete;
    Thread(Function, Argument = 0, Criterion = Criterion::NORMAL, Domain = Domain::USER);
    ~Thread();

    static void init();
    static void run();
    static void sleep(List *, Spin *);
    static void wakeup(List *);
    static void yield();
    static void reschedule();
    static void onTick();
    static void exit();
    static Thread *running();
    void join();

  private:
    static void entry(Function, Argument);
    static void dispatch(Thread *, Thread *, Spin * = 0);
    static Return idle(Argument);
    static void epilogue();

  private:
    Chunk stack_;
    Chunk kstack_;
    Node node_;
    volatile State state_;
    Context context_;
    Domain domain_;

  private:
    static constinit inline Scheduler s_scheduler;
    static inline volatile unsigned int s_count;
    static inline Thread *volatile s_previous[Traits<CPU>::Active];
    static inline Spin *volatile s_spin[Traits<CPU>::Active];
};

} // namespace QUARK
