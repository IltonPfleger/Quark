#pragma once

#include <Mutex.hpp>
#include <Semaphore.hpp>
#include <Traits.hpp>
#include <machine/Machine.hpp>
#include <network/NetworkBuffer.hpp>
#include <utility/collections/FIFO.hpp>

namespace QUARK {

template <typename DEVICE> class VirtualSwitch : public DEVICE::Observer, public DEVICE::Observed {
  public:
    VirtualSwitch()
        : device_(*DEVICE::instance()) {
        device_.attach(this);
    }

    NetworkBuffer *alloc(size_t length) { return device_.alloc(length); }

    void free(NetworkBuffer *buffer) { device_.free(buffer); }

    int send(NetworkBuffer *buffer) {
        lock_.acquire();

        this->notify(buffer);

        lock_.release();

        return device_.send(buffer);
    }

    void update(const NetworkBuffer *buffer) override {
        lock_.acquire();
        this->notify(buffer);
        lock_.release();
    }

    static auto instance() {
        static VirtualSwitch instance;
        return &instance;
    }

  private:
    DEVICE &device_;
    Mutex lock_;
};

} // namespace QUARK
