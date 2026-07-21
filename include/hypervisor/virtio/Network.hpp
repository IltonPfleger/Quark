#pragma once

#include <Traits.hpp>
#include <hypervisor/VirtualMachine.hpp>
#include <hypervisor/virtio/Handler.hpp>
#include <hypervisor/virtio/Queue.hpp>
#include <memory/Heap.hpp>
#include <network/NetworkDevice.hpp>
#include <utility/WorkerManager.hpp>

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
          owner_(owner) {
        device_->attach(this);
    }

    uint32_t configuration(uint32_t) { return 0; }

    void notify(uint32_t source) {
        if (source != 1) return;
        WorkerManager::schedule(worker, this);
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
    static void worker(void *pointer) {
        auto *self = reinterpret_cast<Network *>(pointer);

        self->tx_.notifiable(false);

        bool interrupt = false;
        while (self->tx_.available()) {
            uint32_t head = self->tx_.alloc();
            size_t length = self->process(head);
            self->tx_.free(head, length);
            interrupt = true;
        }

        if (self->tx_.notifiable() && interrupt) {
            self->interrupt();
            self->owner_.interrupt(IRQ);
        }

        self->tx_.notifiable(true);
    }

    size_t process(int head) {
        RingDescriptor *descriptor = tx_.descriptor(head);

        if (descriptor->flags & VRING_DESC_F_NEXT) {
            return fragmented(head);
        }

        size_t descriptors = descriptor->length;
        if (descriptor->length <= sizeof(NetworkHeader)) {
            return descriptors;
        }

        size_t payload = descriptor->length - sizeof(NetworkHeader);

        auto *data = reinterpret_cast<uint8_t *>(descriptor->address) + sizeof(NetworkHeader);

        NetworkBuffer *buffer = device_->alloc(payload);

        memcpy(buffer->start(), data, payload);

        device_->send(buffer);

        return descriptors;
    }

    size_t fragmented(int head) {
        size_t payload     = 0;
        size_t descriptors = 0;
        size_t count       = 0;
        int current        = head;

        RingDescriptor *descriptor = tx_.descriptor(current);
        descriptors += descriptor->length;

        if (descriptor->length >= sizeof(NetworkHeader)) {
            size_t total = descriptor->length - sizeof(NetworkHeader);
            payload      = total;
        }

        RingDescriptor *first = descriptor;

        while (descriptor->flags & VRING_DESC_F_NEXT) {
            assert(count < MaximumNumberOfDescriptors);
            current    = descriptor->next;
            descriptor = tx_.descriptor(current);

            descriptors += descriptor->length;
            payload += descriptor->length;
            count++;
        }

        if (payload == 0) {
            return descriptors;
        }

        NetworkBuffer *buffer = device_->alloc(payload);

        assert(buffer);

        buffer->shrink(buffer->offset());
        buffer->rewind(buffer->offset());
        auto *destination = reinterpret_cast<uint8_t *>(buffer->start());

        descriptor = first;

        if (descriptor->length >= sizeof(NetworkHeader)) {
            auto *data      = reinterpret_cast<uint8_t *>(descriptor->address) + sizeof(NetworkHeader);
            uint32_t length = descriptor->length - sizeof(NetworkHeader);
            memcpy(destination, data, length);
            destination += length;
        }

        while (descriptor->flags & VRING_DESC_F_NEXT) {
            head       = descriptor->next;
            descriptor = tx_.descriptor(head);
            auto *data = reinterpret_cast<uint8_t *>(descriptor->address);
            memcpy(destination, data, descriptor->length);
            destination += descriptor->length;
        }

        device_->send(buffer);

        return descriptors;
    }

  public:
    static constexpr uintptr_t Address                 = ADDRESS;
    static constexpr size_t Size                       = sizeof(LegacyHeader);
    static constexpr size_t MaximumNumberOfDescriptors = 1024;

  private:
    DEVICE *device_;
    VirtualMachine &owner_;

    Queue tx_;
    Queue rx_;
    Queue *queues_[2] = {&rx_, &tx_};

    Configuration configuration_;
};

} // namespace QUARK::virtio
