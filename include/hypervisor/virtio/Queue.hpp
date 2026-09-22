#pragma once

#include <hypervisor/virtio/flags.hpp>
#include <types.hpp>
#include <utility/Debug.hpp>

namespace QUARK::virtio {

struct RingDescriptor {
  uint64_t address;
  uint32_t length;
  uint16_t flags;
  uint16_t next;
} __attribute__((packed));

struct RingAvailable {
  uint16_t flags;
  uint16_t index;
  uint16_t *ring() {
    return reinterpret_cast<uint16_t *>(reinterpret_cast<uintptr_t>(this) + 4);
  };
} __attribute__((packed));

struct RingUsedElement {
  uint32_t id;
  uint32_t length;
};

struct RingUsed {
  uint16_t flags;
  uint16_t index;
  RingUsedElement *ring() {
    return reinterpret_cast<RingUsedElement *>(
        reinterpret_cast<uintptr_t>(this) + 4);
  };
} __attribute__((packed));

class Queue {
public:
  Queue() = default;

  Queue(uintptr_t address, uint32_t size, uint32_t alignament)
      : address_(address), size_(size), last_(0) {
    assert(size > 0 && (size & (size - 1)) == 0);
    assert(alignament > 0 && (alignament & (alignament - 1)) == 0);
    descriptors_ = reinterpret_cast<RingDescriptor *>(address);
    address += sizeof(RingDescriptor) * size;
    available_ = reinterpret_cast<RingAvailable *>(address);
    address += sizeof(uint16_t) * 2;
    address += sizeof(uint16_t) * size;
    address += sizeof(uint16_t);
    address = align(address, alignament);
    used_ = reinterpret_cast<RingUsed *>(address);
    notifiable(true);
  }

  int alloc() {
    if (!available_)
      return -1;

    uint16_t i = *static_cast<volatile uint16_t *>(&available_->index);

    if (last_ == i)
      return -1;

    return available_->ring()[CPU::Atomic::finc(last_) % size_];
  }

  RingDescriptor *descriptor(uint32_t id) {
    assert(id < size_);
    return &descriptors_[id];
  }

  bool interruptible() const {
    if (!available_)
      return false;
    return !(*static_cast<volatile uint16_t *>(&available_->flags) &
             VRING_AVAIL_F_NO_INTERRUPT);
  }

  void notifiable(bool value) {
    if (!used_)
      return;
    if (value)
      *static_cast<volatile uint16_t *>(&used_->flags) &=
          ~VRING_USED_F_NO_NOTIFY;
    else
      *static_cast<volatile uint16_t *>(&used_->flags) |=
          VRING_USED_F_NO_NOTIFY;
    CPU::mbw();
  }

  void free(unsigned int id, unsigned int length = 0) {
    assert(id < size_);
    assert(used_);

    uint16_t ticket = CPU::Atomic::finc(head_);
    uint16_t current = ticket % size_;

    volatile RingUsedElement *element = &used_->ring()[current];
    element->id = id;
    element->length = length;

    while (*static_cast<volatile uint16_t *>(&used_->index) != ticket)
      CPU::mbr();

    CPU::mbw();
    used_->index = ticket + 1;
  }

  uintptr_t address() const { return address_; }

  static constexpr uintptr_t align(uintptr_t address, size_t alignament) {
    return (address + alignament - 1) & ~(alignament - 1);
  }

  static constexpr uintptr_t size(size_t size, size_t alignament) {
    uintptr_t address = 0;
    address += size * sizeof(RingDescriptor);
    address += sizeof(uint16_t) * 2;    // RingAvailable (Flags + Index)
    address += sizeof(uint16_t) * size; // Ring
    address += sizeof(uint16_t);        // Event
    address = align(address, alignament);
    address += sizeof(uint16_t) * 2;           // RingUsed (Flags + Index)
    address += sizeof(RingUsedElement) * size; // Ring
    address += sizeof(uint16_t);               // Event
    return address;
  }

private:
  const uintptr_t address_ = 0;
  const uint32_t size_ = 0;
  uint16_t head_ = 0;
  uint16_t last_ = 0;
  RingDescriptor *descriptors_ = nullptr;
  RingAvailable *available_ = nullptr;
  RingUsed *used_ = nullptr;
};

} // namespace QUARK::virtio
