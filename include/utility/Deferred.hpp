#pragma once

#include <Semaphore.hpp>
#include <Spin.hpp>
#include <utility/Atomic.hpp>
#include <utility/collections/FIFO.hpp>

namespace QUARK {

class Deferred {
  public:
    struct Work;

  private:
    using Element = collections::Node<Work &>;
    using List    = collections::FIFO<Element, Spin>;

  public:
    class Work : public Element {
      public:
        Work(void (*function)(void *), void *argument)
            : Node(*this),
              function_(function),
              argument_(argument),
              pending_(0),
              queued_(false) {}

        void increment(List &list) {
            CPU::IRQ::Guard _;

            lock_.acquire();

            pending_++;

            bool enqueue = false;

            if (!queued_) {
                queued_ = true;
                enqueue = true;
            }

            lock_.release();

            if (enqueue) list.insert(this);
        }

        void decrement(List &list) {
            function_(argument_);

            {
                bool enqueue = false;

                CPU::IRQ::Guard _;

                lock_.acquire();

                pending_--;

                if (pending_ > 0) {
                    enqueue = true;
                } else {
                    queued_ = false;
                }

                lock_.release();

                if (enqueue) list.insert(this);
            }
        }

      private:
        void (*function_)(void *);
        void *argument_;
        int pending_;
        volatile bool queued_;
        Spin lock_;
    };

  public:
    static void init() {
        for (auto &i : managers_) {
            i = new Deferred();
        }
    }

    static bool schedule(Work &work) {
        size_t i       = 0;
        size_t counter = counter_.finc();
        for (; i < Traits<Deferred>::Threads; i++) {
            Deferred *manager = managers_[(i + counter) % Traits<Deferred>::Threads];
            if (manager->enqueue(work)) {
                return true;
            }
        }
        return false;
    }

  private:
    Deferred()
        : running_(true),
          thread_(dispatcher, this) {}

    ~Deferred() {
        running_ = false;
        // pending_.v();
        thread_.join();
    }

    bool enqueue(Work &work) {
        if (!running_) return false;
        work.increment(workers_);
        // pending_.v();
        return true;
    }

    static void *dispatcher(void *pointer) {
        Deferred *self = reinterpret_cast<Deferred *>(pointer);

        while (1) {
            // self->pending_.p();

            if (!self->running_) break;

            Element *element;
            {
                CPU::IRQ::Guard _;
                element = self->workers_.remove();
            }

            if (!element) continue;

            Work &work = element->value;
            work.decrement(self->workers_);
        }

        return nullptr;
    }

  private:
    volatile bool running_;

    // Semaphore pending_;
    List workers_;

    Thread thread_;

  private:
    static inline Atomic<size_t> counter_ = 0;
    static inline Deferred *managers_[Traits<Deferred>::Threads];
};

} // namespace QUARK
