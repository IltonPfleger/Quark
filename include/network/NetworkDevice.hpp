#pragma once

#include <Semaphore.hpp>
#include <Thread.hpp>
#include <network/NetworkBuffer.hpp>
#include <utility/Atomic.hpp>
#include <utility/Observer.hpp>

namespace QUARK {

class NetworkDevice : public Observed<const NetworkBuffer *> {
  public:
    using Observed = QUARK::Observed<const NetworkBuffer *>;
    using Observer = QUARK::Observer<const NetworkBuffer *>;
    using Observed::notify;

    // protected:
    //   void notify() { semaphore_.v(); }

  public:
    virtual ~NetworkDevice()              = default;
    virtual int send(NetworkBuffer *)     = 0;
    virtual void free(NetworkBuffer *)    = 0;
    virtual NetworkBuffer *alloc(size_t)  = 0;
    virtual NetworkBuffer *receive()      = 0;
    virtual void release(NetworkBuffer *) = 0;

    //  void init() {
    //      running_ = true;
    //      thread_  = new Thread(worker, this, {Thread::Criterion::NORMAL, Traits<CPU>::BSP});
    //  }

    //  void stop() {
    //      running_ = false;
    //      semaphore_.v();
    //      delete thread_;
    //  }

    // private:
    //   static void *worker(void *argument);

    // private:
    //  Semaphore semaphore_   = 0;
    //  Thread *thread_        = nullptr;
    //  volatile bool running_ = false;
};

} // namespace QUARK
