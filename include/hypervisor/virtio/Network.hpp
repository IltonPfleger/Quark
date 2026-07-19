#pragma once

#include <Semaphore.hpp>
#include <Thread.hpp>
#include <Traits.hpp>
#include <hypervisor/VirtualMachine.hpp>
#include <hypervisor/virtio/Handler.hpp>
#include <hypervisor/virtio/Queue.hpp>
#include <memory/Heap.hpp>
#include <network/NetworkDevice.hpp>

namespace QUARK::virtio {

template <typename DEVICE, uintptr_t ADDRESS, uint32_t IRQ> class Network : public Handler, public DEVICE::Observer {
    typedef unsigned char NetworkHeader[10];

    struct Configuration {
        uint8_t mac[6];
        uint16_t status;
        uint16_t pairs;
        uint16_t mtu;
    };

    friend Handler;

  public:
    Network(VirtualMachine &owner)
        : Handler(1, 0, MaximumNumberOfDescriptors),
          device_(DEVICE::instance()),
          owner_(owner),
          running_(true),
          thread_(entry, this) {
        device_->attach(this);
    }

    ~Network() {
        running_ = false;
        semaphore_.v();
        thread_.join();
        device_->detach(this);
    }

    uint32_t configuration(uint32_t offset) {
        QUARK::Console::println("OFFSET: ", offset);
        return 0;
    }

    void notify(uint32_t source) {
        if (source != 1) return;
        semaphore_.v();
    }

    void update(const NetworkBuffer *buffer) override {
        auto data = buffer->start();
        auto size = buffer->length();

        if (!rx_.available()) return;

        int id = rx_.alloc();

        auto *descriptor = rx_.descriptor(id);

        auto *destination = reinterpret_cast<uint8_t *>(descriptor->address);

        assert(descriptor->length >= size + sizeof(NetworkHeader));
        memset(destination, 0, sizeof(NetworkHeader));
        memcpy(destination + sizeof(NetworkHeader), data, size);

        descriptor->length = size + sizeof(NetworkHeader);

        rx_.free(id, descriptor->length);

        if (rx_.notifiable()) {
            this->interrupt();
            owner_.interrupt(IRQ);
        }
    }

  private:
    static void *entry(void *self) { return reinterpret_cast<Network *>(self)->worker(); }

    void *worker() {

        while (true) {
            tx_.notifiable(true);

            semaphore_.p();

            tx_.notifiable(false);

            if (!running_) break;

            while (tx_.available()) {
                uint32_t head = tx_.alloc();
                size_t length = process(head);
                tx_.free(head, length);
            }

            if (tx_.notifiable()) {
                this->interrupt();
                owner_.interrupt(IRQ);
            }
        }
        return nullptr;
    }

    size_t process(int head) {
        size_t total = 0;
        int current  = head;
        size_t count = 0;

        RingDescriptor *descriptor = tx_.descriptor(current);
        total += descriptor->length;

        if (descriptor->length >= sizeof(NetworkHeader)) {
            auto *data      = reinterpret_cast<uint8_t *>(descriptor->address) + sizeof(NetworkHeader);
            uint32_t length = descriptor->length - sizeof(NetworkHeader);
            send(data, length);
        }

        while (descriptor->flags & VRING_DESC_F_NEXT) {
            assert(count < MaximumNumberOfDescriptors);
            current    = descriptor->next;
            descriptor = tx_.descriptor(current);
            total += descriptor->length;
            auto *data = reinterpret_cast<uint8_t *>(descriptor->address);
            send(data, descriptor->length);
            count++;
        }

        return total;
    }

    void send(const uint8_t *data, uint32_t length) {
        if (length == 0) return;
        NetworkBuffer *buffer = device_->alloc(length);
        if (buffer) {
            buffer->shrink(buffer->offset());
            buffer->rewind(buffer->offset());
            memcpy(buffer->start(), data, length);
            device_->send(buffer);
        }
    }

  public:
    static constexpr uintptr_t Address                 = ADDRESS;
    static constexpr size_t Size                       = sizeof(LegacyHeader);
    static constexpr size_t MaximumNumberOfDescriptors = 512;

  private:
    DEVICE *device_;
    VirtualMachine &owner_;

    Semaphore semaphore_;

    volatile bool running_;
    Thread thread_;

    Configuration configuration_;
    Queue tx_;
    Queue rx_;
    Queue *queues_[2] = {&rx_, &tx_};
};

} // namespace QUARK::virtio
