#pragma once

#include <Mutex.hpp>
#include <Traits.hpp>
#include <hypervisor/VirtualMachine.hpp>
#include <hypervisor/virtio/Handler.hpp>
#include <hypervisor/virtio/Queue.hpp>
#include <memory/Heap.hpp>
#include <network/NetworkDevice.hpp>
#include <utility/Deferred.hpp>

namespace QUARK::virtio {

template <typename DEVICE, uintptr_t ADDRESS, uint32_t IRQ>
class Network : public Handler, public DEVICE::Observer {
  using Header = uint8_t[10];
  friend Handler;

public:
  explicit Network(VirtualMachine &owner)
      : Handler(1, VIRTIO_NET_F_MTU, MaximumNumberOfDescriptors),
        device_(DEVICE::instance()), owner_(owner), deferred_(drain, this) {
    device_.attach(this);
  }

  ~Network() { device_.detach(this); }

  uint32_t configuration(uint32_t offset) {
    return offset == 10 ? device_.mtu() : 0;
  }

  void notify(uint32_t source) {
    if (source == Tx)
      Deferred::schedule(deferred_);
  }

  void update(const NetworkBuffer *buffer) override {
    const uint8_t *data = buffer->start();
    const size_t size = buffer->capacity();

    const int id = queues_[Rx].alloc();

    if (id < 0)
      return;

    RingDescriptor *descriptor = queues_[Rx].descriptor(id);
    uint8_t *destination = reinterpret_cast<uint8_t *>(descriptor->address);

    assert(descriptor->length >= size + sizeof(Header));
    memset(destination, 0, sizeof(Header));
    memcpy(destination + sizeof(Header), data, size);

    descriptor->length = size + sizeof(Header);
    queues_[Rx].free(id, descriptor->length);

    signal(queues_[Rx]);
  }

private:
  static void drain(void *pointer) {
    Network &self = *reinterpret_cast<Network *>(pointer);
    bool sent = false;

    while (true) {
      const int head = self.queues_[Tx].alloc();

      if (head < 0) {
        break;
      }

      self.queues_[Tx].free(head, self.transmit(head));
      sent = true;

      CPU::mb();
    }

    if (sent)
      self.signal(self.queues_[Tx]);
  }

  size_t transmit(int head) {
    size_t total = 0;
    for (int current = head;;) {
      RingDescriptor *descriptor = queues_[Tx].descriptor(current);
      total += descriptor->length;
      if (!(descriptor->flags & VRING_DESC_F_NEXT))
        break;
      current = descriptor->next;
    }

    if (total <= sizeof(Header))
      return total;

    NetworkBuffer *buffer = device_.alloc(total - sizeof(Header));
    buffer->rewind(buffer->offset());
    buffer->shrink(buffer->offset());

    auto *destination = reinterpret_cast<uint8_t *>(buffer->start());
    size_t skip = sizeof(Header);

    for (int current = head;;) {
      RingDescriptor *descriptor = queues_[Tx].descriptor(current);
      auto *source = reinterpret_cast<uint8_t *>(descriptor->address);
      size_t bytes = descriptor->length;

      if (skip > 0) {
        const size_t consumed = skip < bytes ? skip : bytes;
        source += consumed;
        bytes -= consumed;
        skip -= consumed;
      }

      if (bytes > 0) {
        memcpy(destination, source, bytes);
        destination += bytes;
      }

      if (!(descriptor->flags & VRING_DESC_F_NEXT))
        break;
      current = descriptor->next;
    }

    device_.send(buffer);
    return total;
  }

  void signal(Queue &queue) {
    if (queue.interruptible()) {
      this->interrupt();
      owner_.interrupt(IRQ);
    }
  }

public:
  static constexpr uintptr_t Address = ADDRESS;
  static constexpr size_t MaximumNumberOfDescriptors = 1024;
  static constexpr uint32_t Rx = 0;
  static constexpr uint32_t Tx = 1;

private:
  DEVICE &device_;
  VirtualMachine &owner_;
  Deferred deferred_;
  Queue queues_[2];
};

} // namespace QUARK::virtio
