#ifndef __QUARK_UTILITY_DEFERRED__
#define __QUARK_UTILITY_DEFERRED__

#include <Semaphore.hpp>
#include <Spin.hpp>
#include <Traits.hpp>
#include <utility/Atomic.hpp>
#include <utility/collections/MPSC.hpp>

namespace QUARK {

class Deferred {
public:
  struct Work;

private:
  using Element = collections::Node<Work *>;
  using List = collections::MPSC<Element>;

public:
  class Work : public Element {
  public:
    Work(void (*function)(void *), void *argument)
        : Node(this), function_(function), argument_(argument), pending_(0) {}

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
    if constexpr (Threads != 0)
      return next_.finc() % Threads;
    return 0;
  }

  static void init() {
    if (ussing_.finc() != 0)
      return;

    for (auto &i : managers_)
      i = new Deferred();
  }

  static void destroy() {
    if (ussing_.fdec() != 1)
      return;

    for (auto &i : managers_)
      delete i;
  }

  static bool schedule(Work &work) {
    for (size_t i = 0; i < Threads; i++) {
      Deferred *manager = managers_[id()];
      if (manager->enqueue(work))
        return true;
    }
    return false;
  }

private:
  Deferred()
      : running_(true),
        thread_(dispatcher, this,
                Thread::Criterion(Thread::Criterion::NORMAL, id())) {}

  ~Deferred() {
    running_ = false;
    pending_.v();
    thread_.join();
  }

  bool enqueue(Work &work) {
    if (!running_)
      return false;
    work.increment(workers_, pending_);
    return true;
  }

  static void *dispatcher(void *pointer) {
    Deferred *self = reinterpret_cast<Deferred *>(pointer);

    while (1) {
      self->pending_.p();

      if (!self->running_)
        break;

      Element *element = self->workers_.remove();

      if (!element)
        continue;

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
  static constexpr size_t Threads = Traits<Deferred>::Threads;

private:
  static inline Atomic<size_t> next_ = 0;
  static inline Atomic<size_t> ussing_ = 0;
  static inline Meta::Array<Threads, Deferred *> managers_;
};

} // namespace QUARK

#endif
