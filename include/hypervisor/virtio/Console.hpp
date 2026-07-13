#pragma once

#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <hypervisor/virtio/Handler.hpp>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>
#include <utility/Observer.hpp>

namespace QUARK {

namespace virtio {

template <typename DEVICE, uintptr_t ADDRESS, uint32_t IRQ>
class Console : public Handler, public Observer<const unsigned char *, size_t> {
    friend Handler;

  public:
    Console(VirtualMachine &owner)
        : Handler(3, 1 << 27, 32),
          device_(DEVICE::instance()),
          owner_(owner) {
        device_->attach(this);
    }

    void notify(unsigned int source) {
        if (source != TX) return;

        auto &queue = this->queues_[TX];

        while (queue.available()) {
            int head      = queue.alloc();
            size_t length = process(queue, head);
            queue.free(head, length);
        }
    }

    void update(const unsigned char *buffer, size_t size) override {
        Queue &queue = this->queues_[RX];

        if (!queue.available()) return;

        int id            = queue.alloc();
        auto *descriptor  = queue.get(id);
        auto *destination = reinterpret_cast<unsigned char *>(descriptor->address);

        descriptor->length = size;
        descriptor->flags  = 0;

        memcpy(destination, buffer, size);

        queue.free(id, size);
        this->interrupt() |= 1;
        owner_.interrupt(IRQ);
    }

    size_t process(Queue &queue, int head) {
        size_t total = 0;
        int current  = head;

        RingDescriptor *descriptor = queue.get(current);

        total += print(descriptor);

        while (descriptor->flags & 0x1) {
            current    = descriptor->next;
            descriptor = queue.get(current);
            total += print(descriptor);
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
    static constexpr uint32_t TX       = 1;
    static constexpr uint32_t RX       = 0;

  private:
    DEVICE *device_;
    VirtualMachine &owner_;
};

} // namespace virtio

} // namespace QUARK
