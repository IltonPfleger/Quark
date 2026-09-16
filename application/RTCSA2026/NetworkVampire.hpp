#ifndef __QUARK_NETWORK_VAMPIRE__
#define __QUARK_NETWORK_VAMPIRE__

#include <Mutex.hpp>
#include <Semaphore.hpp>
#include <Thread.hpp>
#include <architecture/CPU.hpp>
#include <architecture/Timer.hpp>
#include <libraries/libc/string.h>
#include <network/NetworkBuffer.hpp>

namespace QUARK {

template <typename DEVICE> class NetworkVampire : DEVICE::Observer {
  public:
    NetworkVampire()
        : device_(DEVICE::instance()),
          done_(true) {
        device_->attach(this);
        new Thread(worker, this);
    }

    void update(const NetworkBuffer *buffer) override {
        lock_.acquire();

        if (!done_ || buffer->length() >= 1522) {
            lock_.release();
            return;
        }

        memcpy(buffer_, buffer->start(), buffer->length());
        length_ = buffer->length();

        done_ = false;

        lock_.release();

        p_.v();
    }

    static void *worker(void *pointer) {
        NetworkVampire *self = reinterpret_cast<NetworkVampire *>(pointer);

        static uint8_t buffer[1522];
        size_t length;

        while (1) {
            self->p_.p();

            self->lock_.acquire();
            length = self->length_;
            memcpy(buffer, self->buffer_, length);
            self->lock_.release();

            for (int i = 0; i < 8; i++) {
                NetworkBuffer nb(buffer, 0, length, nullptr);
                self->device_->clone(&nb);
                // NetworkBuffer *clone = self->device_->alloc(length);
                // memcpy(clone->start(), buffer, length);
                // device_->clone(clone);
                // self->device_->send(clone);
            }

            self->done_ = true;
        }
        return nullptr;
    }

  private:
    DEVICE *device_;
    Mutex lock_;
    Semaphore p_;
    unsigned char buffer_[1522];
    size_t length_;
    bool done_;
};

} // namespace QUARK

#endif
