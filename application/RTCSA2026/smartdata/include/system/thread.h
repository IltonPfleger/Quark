#pragma once

#include <PeriodicThread.hpp>

class Thread : public QUARK::Thread {
  public:
    using QUARK::Thread::Thread;
    static Thread *running() { static_cast<Thread *>(QUARK::Thread::running()); }
};

class Periodic_Thread : public QUARK::PeriodicThread {
  public:
    using QUARK::PeriodicThread::PeriodicThread;
};

// class Periodic_Thread : Thread {
//   public:
//     Periodic_Thread(Microsecond p, Microsecond d, Microsecond delay, void *(*function)(void *), void *argument)
//         : Thread(function, argument), _period(p), _deadline(d), _next_activation(TSC::time_stamp() + _period) {}
//
//     Periodic_Thread(Microsecond p, Microsecond delay, void *(*function)(void *), void *argument)
//         : Thread(function, argument), _period(p), _next_activation(TSC::time_stamp() + _period) {}
//
//     const Microsecond &period() { return _period; }
//     void period(const Microsecond &p) { _period = p; }
//
//     static bool wait_next(Microsecond activation = 0) {
//         Thread *running = Thread::running();
//
//         Periodic_Thread *t = static_cast<Periodic_Thread *>(running);
//
//         Microsecond now = TSC::time_stamp();
//
//         if (activation > 0) {
//             t->_next_activation = activation;
//         }
//
//         bool deadline_met = true;
//
//         if (t->_next_activation > now) {
//             QUARK::Alarm::udelay(t->_next_activation - now);
//         } else {
//             deadline_met = false;
//         }
//
//         t->_next_activation = t->_next_activation + t->_period;
//
//         return deadline_met;
//     }
//
//   private:
//     Microsecond _period;
//     Microsecond _deadline;
//     Microsecond _next_activation;
// };

class Alarm {
  public:
    Alarm(const Microsecond &time, Handler *handler, UInt32 times = 1) { db<Alarm>(TRC) << "Alarm::Alarm()" << endl; }
    static void delay(const Microsecond &time) { db<Alarm>(TRC) << "Alarm::delay()" << endl; }
    void reset() { db<Alarm>(TRC) << "Alarm::reset()" << endl; }
};
