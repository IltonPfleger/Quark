#pragma once

#include <Alarm.hpp>
#include <Semaphore.hpp>
#include <Thread.hpp>
#include <architecture/Timer.hpp>

namespace QUARK {

class PeriodicThread : private Semaphore, public Thread {
    using Thread::Argument;
    using Thread::Criterion;
    using Thread::Function;

    struct Arguments {
        Function function;
        Argument argument;
    };

    static void *dispatch(void *argument) {
        PeriodicThread *self = static_cast<PeriodicThread *>(argument);
        self->p();
        self->next_ = Timer::now() + self->period_;
        self->arguments_.function(self->arguments_.argument);
        return 0;
    }

  public:
    PeriodicThread(Microsecond t, Function f, Argument a, Criterion c = Criterion::NORMAL)
        : Semaphore(0),
          Thread(dispatch, this, c),
          arguments_(f, a),
          period_(t) {
        this->v();
    }

    static void wait(Microsecond = 0) {
        PeriodicThread *self = static_cast<PeriodicThread *>(Thread::running());
        Alarm(self->next_);
        self->next_ = self->next_ + self->period_;
    }

    Microsecond period() { return period_; }

    void period(Microsecond period) { period_ = period; }

  private:
    Arguments arguments_;
    Microsecond period_;
    Microsecond next_;
};

} // namespace QUARK
