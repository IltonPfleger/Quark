#pragma once

#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <hypervisor/virtio/Flags.hpp>
#include <hypervisor/virtio/Handler.hpp>
#include <hypervisor/virtio/Queue.hpp>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>
#include <utility/Observer.hpp>

namespace QUARK {

namespace virtio {

template <typename DEVICE, uintptr_t ADDRESS, uint32_t IRQ> class Console : public Handler, public Observer<const unsigned char *, size_t> {
    friend Handler;

  public:
    Console(VirtualMachine &owner)
        : Handler(3, 1 << 27, N),
          device_(DEVICE::instance()),
          owner_(owner) {
        device_->attach(this);
    }

    uint32_t config(uint32_t) { return 0; }

    void notify(unsigned int source) {
        if (source != 1) return;

        while (tx_.available()) {
            int head      = tx_.alloc();
            size_t length = process(head);
            tx_.free(head, length);
        }
    }

    void update(const unsigned char *buffer, size_t size) override {
        if (!rx_.available()) return;

        int id            = rx_.alloc();
        auto *descriptor  = rx_.get(id);
        auto *destination = reinterpret_cast<unsigned char *>(descriptor->address);

        descriptor->length = size;
        descriptor->flags  = 0;

        memcpy(destination, buffer, size);

        rx_.free(id, size);
        this->interrupt() |= 1;
        owner_.interrupt(IRQ);
    }

    size_t process(int head) {
        size_t total = 0;
        size_t count = 0;
        int current  = head;

        RingDescriptor *descriptor = tx_.get(current);

        total += print(descriptor);

        while (descriptor->flags & VRING_DESC_F_NEXT) {
            assert(count < N);
            current    = descriptor->next;
            descriptor = tx_.get(current);
            total += print(descriptor);
            count++;
        }

        return total;
    }

    size_t print(RingDescriptor *descriptor) {
        char *data      = reinterpret_cast<char *>(descriptor->address);
        uint32_t length = descriptor->length;
        for (uint32_t j = 0; j < descriptor->length; j++)
            QUARK::Console::print(data[j]);
        return length;
    }

  public:
    static constexpr uintptr_t Address = ADDRESS;
    static constexpr size_t N          = 32;

  private:
    DEVICE *device_;
    VirtualMachine &owner_;

    Queue tx_;
    Queue rx_;
    Queue *queues_[2] = {&rx_, &tx_};
};

} // namespace virtio

} // namespace QUARK
