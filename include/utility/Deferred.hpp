#ifndef __QUARK_DEFERRED__
#define __QUARK_DEFERRED__

#include <Semaphore.hpp>
#include <Spin.hpp>
#include <utility/Atomic.hpp>
#include <utility/collections/MPSC.hpp>

namespace QUARK {

class Deferred {
  public:
    struct Work;

  private:
    using Element = collections::Node<Work *>;
    using List    = collections::MPSC<Element>;

  public:
    class Work : public Element {
      public:
        Work(void (*function)(void *), void *argument)
            : Node(this),
              function_(function),
              argument_(argument),
              pending_(0) {}

        void increment(List &list, Semaphore &semaphore) {
            assert(pending_ >= 0);

            if (pending_.finc() == 0) {
                list.insert(this);
                semaphore.v();
            }
        }

        void decrement(List &list, Semaphore &semaphore) {
            function_(argument_);

            assert(pending_ > 0);

            if (pending_.fdec() > 1) {
                list.insert(this);
                semaphore.v();
            }
        }

      private:
        void (*function_)(void *);
        void *argument_;
        Atomic<int> pending_;
    };

  public:
    static size_t id() {
        if constexpr (Traits<Deferred>::Threads != 0) return counter_.finc() % Traits<Deferred>::Threads;
        return 0;
    }

    static void init() {
        for (auto &i : managers_) {
            i = new Deferred();
        }
    }

    static bool schedule(Work &work) {
        for (size_t i = 0; i < Traits<Deferred>::Threads; i++) {
            Deferred *manager = managers_[id()];
            if (manager->enqueue(work)) return true;
        }
        return false;
    }

  private:
    Deferred()
        : running_(true),
          thread_(dispatcher, this, Thread::Criterion(Thread::Criterion::NORMAL, id())) {}

    ~Deferred() {
        running_ = false;
        pending_.v();
        thread_.join();
    }

    bool enqueue(Work &work) {
        if (!running_) return false;
        work.increment(workers_, pending_);
        return true;
    }

    static void *dispatcher(void *pointer) {
        Deferred *self = reinterpret_cast<Deferred *>(pointer);

        while (1) {
            self->pending_.p();

            if (!self->running_) break;

            Element *element = self->workers_.remove();

            if (!element) continue;

            Work *work = element->value;

            work->decrement(self->workers_, self->pending_);
        }

        return nullptr;
    }

  private:
    volatile bool running_;

    Semaphore pending_;

    List workers_;

    Thread thread_;

  private:
    static inline Atomic<size_t> counter_ = 0;
    static inline Meta::Array<Traits<Deferred>::Threads, Deferred *> managers_;
};

} // namespace QUARK

#endif
