#ifndef __QUARK_UTILITY_DEFERRED__
#define __QUARK_UTILITY_DEFERRED__

#include <Traits.hpp>
#include <synchronization/Semaphore.hpp>
#include <utility/Atomic.hpp>
#include <utility/collections/FIFO.hpp>

namespace QUARK {

class Deferred {
  using Element = collections::Node<Deferred *>;
  using List = collections::FIFO<Element, Spin>;
  using Function = void (*)(void *);

  class Worker {
  public:
    Worker(size_t = 0) : running_(true), thread_(dispatcher, this) {}
    ~Worker() { running_ = false; }

    bool insert(Deferred &work) {
      if (!running_)
        return false;

      work.increment(list_, pending_);
      return true;
    }

    static void *dispatcher(void *pointer) {
      Worker *self = reinterpret_cast<Worker *>(pointer);

      while (self->running_) {
        self->pending_.p();

        while (true) {
          Element *element;

          {
            CPU::IRQ::Guard _;
            element = self->list_.remove();
          }

          if (!element)
            break;

          Deferred *work = element->value;
          work->decrement(self->list_, self->pending_);
        }
      }

      return nullptr;
    }

  private:
    volatile bool running_;
    List list_;
    Semaphore pending_;
    Thread thread_;
  };

private:
  void increment(List &list, Semaphore &semaphore) {
    if (stopping_)
      return;

    if (pending_.finc() == 0) {
      {
        CPU::IRQ::Guard _;
        list.insert(&this->element_);
      }
      semaphore.v();
    }
  }

  void decrement(List &list, Semaphore &semaphore) {
    function_(argument_);
    if (pending_.fdec() > 1) {
      {
        CPU::IRQ::Guard _;
        list.insert(&this->element_);
      }
      semaphore.v();
    } else if (stopping_) {
      latch_.v();
    }
  }

public:
  Deferred(Function function = nullptr, void *argument = nullptr)
      : element_(this), function_(function), argument_(argument), pending_(0),
        stopping_(false) {}

  ~Deferred() {
    stopping_ = true;
    if (pending_ == 0)
      return;
    latch_.p();
  }

  static void init() {
    for (size_t i = 0; i < kThreads; ++i)
      workers[i] = new (Heap::SYSTEM) Worker(i);
  }

  static bool schedule(Deferred &work) {
    static size_t next = 0;
    const size_t start = CPU::Atomic::finc(next);

    for (size_t i = 0; i < kThreads; i++) {
      Worker *worker = nullptr;

      if constexpr (kThreads != 0) {
        worker = workers[(start + i) % kThreads];
      }

      if (worker && worker->insert(work)) {
        return true;
      }
    }

    return false;
  }

private:
  Element element_;
  Function function_;
  void *argument_;
  Atomic<int> pending_;
  volatile bool stopping_;
  Semaphore latch_;

  static constexpr size_t kThreads = Traits<Deferred>::Threads;

private:
  static inline Meta::Array<kThreads, Worker *> workers;
};

} // namespace QUARK

#endif
