#ifndef __QUARK_HYPERVISOR_VIRTIO_CONSOLE__
#define __QUARK_HYPERVISOR_VIRTIO_CONSOLE__

#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <hypervisor/virtio/Handler.hpp>
#include <hypervisor/virtio/Queue.hpp>
#include <memory/Heap.hpp>
#include <utility/Console.hpp>
#include <utility/Deferred.hpp>
#include <utility/Observer.hpp>

namespace QUARK::virtio {

template <typename DEVICE, uintptr_t ADDRESS, uint32_t IRQ>
class Console : public Handler, public Observer<const char *, size_t> {
  friend Handler;

  static_assert(Traits<Deferred>::Threads > 0);

  enum { RX, TX };

public:
  Console(VirtualMachine &owner)
      : Handler(3, 1 << 27, kMaximumNumberOfDescriptors),
        device_(*DEVICE::instance()), owner_(owner), deferred_(worker, this) {
    device_.attach(this);
  }

  ~Console() { device_.detach(this); }

  uint32_t configuration(uint32_t) { return 0; }

  void notify(uint32_t source) {
    if (source == 1)
      Deferred::schedule(deferred_);
  }

  static void worker(void *pointer) {
    auto *self = reinterpret_cast<Console *>(pointer);
    while (true) {
      const int head = self->queues_[TX].alloc();

      if (head < 0)
        break;

      self->queues_[TX].free(head, self->process(head));
    }
  }

  void update(const char *buffer, size_t size) override {
    const int id = queues_[RX].alloc();

    if (id < 0)
      return;

    auto *descriptor = queues_[RX].descriptor(id);
    auto *destination = reinterpret_cast<uint8_t *>(descriptor->address);

    descriptor->length = size;
    descriptor->flags = 0;

    memcpy(destination, buffer, size);

    queues_[RX].free(id, size);

    if (queues_[RX].interruptible()) {
      this->interrupt();
      owner_.interrupt(IRQ);
    }
  }

  size_t process(int head) {
    size_t total = 0;
    size_t count = 0;
    int current = head;

    RingDescriptor *descriptor = queues_[TX].descriptor(current);

    total += print(descriptor);

    while (descriptor->flags & VRING_DESC_F_NEXT) {
      assert(count < kMaximumNumberOfDescriptors);
      current = descriptor->next;
      descriptor = queues_[TX].descriptor(current);
      total += print(descriptor);
      count++;
    }

    return total;
  }

  size_t print(RingDescriptor *descriptor) {
    char *data = reinterpret_cast<char *>(descriptor->address);
    uint32_t length = descriptor->length;
    for (uint32_t j = 0; j < descriptor->length; j++)
      QUARK::Console::print(data[j]);
    return length;
  }

public:
  static constexpr size_t kMaximumNumberOfDescriptors = 1024;
  static constexpr uintptr_t Address = ADDRESS;

private:
  DEVICE &device_;
  VirtualMachine &owner_;
  Deferred deferred_;
  Queue queues_[2];
};

} // namespace QUARK::virtio

#endif
