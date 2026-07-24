#pragma once

#include <Semaphore.hpp>
#include <Spin.hpp>
#include <utility/Atomic.hpp>
#include <utility/collections/UnorderedList.hpp>

namespace QUARK {

class Deferred {
  public:
    struct Work;

  private:
    using Element = collections::Node<Work &>;
    using List    = collections::UnorderedList<Element, Spin>;

  public:
    struct Work : public Element {
        Work(void (*function)(void *), void *argument, bool atomic = true)
            : Node(*this),
              function_(function),
              argument_(argument),
              atomic_(atomic),
              lined_(false),
              pending_(false),
              counter_(0) {}

        void decrement(List &list) {
            lock_.acquire();

            lined_ = false;

            if (atomic_ && pending_) {
                lined_ = true;
                list.insert(this);
                lock_.release();
                return;
            }

            if (counter_ == 0) {
                lock_.release();
                return;
            }

            counter_--;

            pending_ = true;

            if (counter_ > 0 && !atomic_) {
                lined_ = true;
                list.insert(this);
            }

            lock_.release();

            function_(argument_);

            lock_.acquire();

            pending_ = false;

            if (counter_ > 0 && !lined_ && atomic_) {
                lined_ = true;
                list.insert(this);
            }

            lock_.release();
        }

        void increment(List &list) {
            lock_.acquire();
            counter_++;
            if (!lined_) {
                lined_ = true;
                list.insert(this);
            }
            lock_.release();
        }

      private:
        void (*function_)(void *);
        void *const argument_;
        const bool atomic_;
        bool lined_;
        bool pending_;
        int counter_;
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
        pending_.v();
        thread_.join();
    }

    bool enqueue(Work &work) {
        if (!running_) return false;
        work.increment(workers_);
        pending_.v();
        return false;
    }

    static void *dispatcher(void *pointer) {
        Deferred *self = reinterpret_cast<Deferred *>(pointer);

        while (1) {
            self->pending_.p();

            Element *element = self->workers_.remove();

            while (element) {
                Work &work = element->value;
                work.decrement(self->workers_);
                element = self->workers_.remove();
            }

            if (!self->running_) break;
        };

        return nullptr;
    }

  private:
  private:
    volatile bool running_;

    Semaphore pending_;
    List workers_;

    Thread thread_;

  private:
    static inline Atomic<size_t> counter_ = 0;
    static inline Deferred *managers_[Traits<Deferred>::Threads];
};

} // namespace QUARK
