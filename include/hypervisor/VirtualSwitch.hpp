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
        : device_(DEVICE::instance()) {
        device_->attach(this);
    }

    NetworkBuffer *alloc(size_t length) { return device_->alloc(length); }

    int send(NetworkBuffer *buffer) {
        size_t length = buffer->length();
        lock_.acquire();
        this->notify(buffer);
        lock_.release();
        device_->send(buffer);
        return length;
    }

    void update(const NetworkBuffer *buffer) override { this->notify(buffer); }

    static auto instance() {
        static VirtualSwitch instance;
        return &instance;
    }

  private:
    DEVICE *device_;
    Mutex lock_;
};

} // namespace QUARK
