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

    int send(NetworkBuffer *buffer, typename DEVICE::Observer *sender = nullptr) {
        size_t length = buffer->length();

        lock_.acquire();

        this->notify(buffer, sender);

        lock_.release();

        device_.send(buffer);

        return length;
    }

    void update(const NetworkBuffer *buffer) override {
        lock_.acquire();
        this->notify(buffer);
        lock_.release();
    }

    void notify(NetworkBuffer *buffer, DEVICE::Observer *excludes) override {
        for (auto *l = this->observers_.head(); l; l = l->next) {
            if (l != excludes) l->value->update(buffer);
        }
    }

    static auto instance() {
        static VirtualSwitch instance;
        return &instance;
    }

  private:
    DEVICE *device_;
    Mutex lock_;
};

} // namespace QUARK
