#pragma once

#include <Semaphore.hpp>
#include <Spin.hpp>
#include <utility/collections/CircularBuffer.hpp>

namespace QUARK {

class WorkerManager {
    struct Worker {
        void (*function)(void *);
        void *argument;
        void operator()() const { function(argument); }
    };

  public:
    static void init() {
        for (auto &i : managers_) {
            i = new WorkerManager();
        }
    }

    template <typename Function, typename Argument> static bool schedule(Function function, Argument argument) {
        Worker worker(function, argument);
        for (auto &i : managers_) {
            if (i->schedule(static_cast<Worker &&>(worker))) {
                return true;
            }
        }
        return false;
    }

  private:
    WorkerManager()
        : running_(true),
          thread_(dispatcher, this) {}

    ~WorkerManager() {
        running_ = false;
        pending_.v();
        thread_.join();
    }

    bool schedule(Worker &&worker) {
        if (running_ && workers_.insert(static_cast<Worker &&>(worker))) {
            pending_.v();
            return true;
        }
        return false;
    }

    static void *dispatcher(void *pointer) {
        WorkerManager *self = reinterpret_cast<WorkerManager *>(pointer);

        while (1) {
            self->pending_.p();
            Worker worker;
            while (self->workers_.remove(worker)) {
                worker();
            }
            if (!self->running_) break;
        };

        return nullptr;
    }

  private:
    using List = collections::CircularBuffer<Worker, Traits<WorkerManager>::Capacity, Spin>;

  private:
    volatile bool running_;

    Semaphore pending_;
    List workers_;

    Thread thread_;

  private:
    static inline WorkerManager *managers_[Traits<WorkerManager>::Threads];
};

} // namespace QUARK
