#ifndef __QUARK_UTILITY_DEFERRED__
#define __QUARK_UTILITY_DEFERRED__

#include <Semaphore.hpp>
#include <Spin.hpp>
#include <Traits.hpp>
#include <utility/Atomic.hpp>
#include <utility/collections/FIFO.hpp>

namespace QUARK {

class Deferred {
public:
  struct Work;

private:
  using Element = collections::Node<Work *>;
  using List = collections::FIFO<Element>;

public:
  class Work : public Element {
    friend Deferred;

  public:
    Work(void (*function)(void *), void *argument)
        : Node(this), function_(function), argument_(argument), pending_(0) {}

  private:
    void increment(List &list, Semaphore &semaphore) {
      if (pending_.finc() == 0) {
        {
          CPU::IRQ::Guard irq;
          list.insert(this);
        }
        semaphore.v();
      }
    }

    void decrement(List &list, Semaphore &semaphore) {
      function_(argument_);
      if (pending_.fdec() > 1) {
        {
          CPU::IRQ::Guard irq;
          list.insert(this);
        }
        semaphore.v();
      }
    }

  private:
    void (*function_)(void *);
    void *argument_;
    Atomic<size_t> pending_;
  };

public:
  static size_t id() {
    if constexpr (Threads != 0)
      return next_.finc() % Threads;
    return 0;
  }

  static void init() {
    if (using_.finc() == 0) {
      for (size_t i = 0; i < Threads; ++i)
        managers_[i] = new Deferred(i);
      initialized_ = true;
    } else {
      while (!initialized_) {
      }
    }
  }

  static void destroy() {
    if (using_.fdec() != 1)
      return;

    initialized_ = false;

    for (size_t i = 0; i < Threads; ++i) {
      Deferred *manager = managers_[i];
      managers_[i] = nullptr;
      delete manager;
    }
  }

  static bool schedule(Work &work) {
    if (!initialized_)
      return false;

    const size_t start = id();

    for (size_t i = 0; i < Threads; i++) {
      Deferred *manager;

      if constexpr (Threads != 0) {
        manager = managers_[(start + i) % Threads];
      } else {
        manager = managers_[0];
      }

      assert(manager);

      if (manager->enqueue(work))
        return true;
    }

    return false;
  }

private:
  Deferred(size_t heart)
      : running_(true),
        thread_(dispatcher, this,
                Thread::Criterion(Thread::Criterion::NORMAL, heart)) {}

  ~Deferred() {
    running_ = false;
    pending_.v();
    thread_.join();
  }

  bool enqueue(Work &work) {
    if (!running_ || !initialized_)
      return false;

    work.increment(workers_, pending_);
    return true;
  }

  static void *dispatcher(void *pointer) {
    Deferred *self = reinterpret_cast<Deferred *>(pointer);

    while (self->running_) {
      self->pending_.p();

      while (1) {
        Element *element;

        {
          CPU::IRQ::Guard irq;
          element = self->workers_.remove();
        }

        if (!element)
          break;

        Work *work = element->value;
        work->decrement(self->workers_, self->pending_);
      }
    }

    return nullptr;
  }

private:
  volatile bool running_;
  Semaphore pending_;
  List workers_;
  Thread thread_;

private:
  static constexpr size_t Threads = Traits<Deferred>::Threads;

private:
  static constinit inline Atomic<size_t> next_ = 0;
  static constinit inline Atomic<size_t> using_ = 0;
  static constinit inline Atomic<bool> initialized_ = false;
  static inline Meta::Array<Threads, Deferred *> managers_;
};

} // namespace QUARK

#endif
