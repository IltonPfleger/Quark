#pragma once

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
    uint16_t *ring() { return reinterpret_cast<uint16_t *>(reinterpret_cast<uintptr_t>(this) + 4); };
} __attribute__((packed));

struct RingUsedElement {
    uint32_t id;
    uint32_t length;
};

struct RingUsed {
    uint16_t flags;
    uint16_t index;
    RingUsedElement *ring() { return reinterpret_cast<RingUsedElement *>(reinterpret_cast<uintptr_t>(this) + 4); };
} __attribute__((packed));

class Queue {
  public:
    Queue() = default;

    Queue(uintptr_t address, uint32_t size, uint32_t alignament)
        : address_(address),
          size_(size),
          last_(0) {

        descriptors_ = reinterpret_cast<RingDescriptor *>(address);

        address += sizeof(RingDescriptor) * size;

        available_ = reinterpret_cast<RingAvailable *>(address);

        address += sizeof(uint16_t) * 2;
        address += sizeof(uint16_t) * size;
        address += sizeof(uint16_t);

        address = align(address, alignament);

        used_ = reinterpret_cast<RingUsed *>(address);
    }

    bool available() {
        if (!available_) return false;
        return last_ != available_->index;
    }

    int alloc() {
        assert(available_);
        return available_->ring()[last_++ % size_];
    }

    RingDescriptor *get(uint32_t id) {
        assert(id < size_);
        return &descriptors_[id];
    }

    void free(unsigned int id, unsigned int length = 0) {
        assert(id < size_);
        uint16_t index              = used_->index % size_;
        used_->ring()[index].id     = id;
        used_->ring()[index].length = length;
        used_->index++;
    }

    uintptr_t address() const { return address_; }

    static constexpr uintptr_t align(uintptr_t address, uint32_t align) { return (address + align - 1) & ~(align - 1); }

    static constexpr uintptr_t size(uint32_t size, uint32_t alignament) {
        uintptr_t address = 0;
        address += size * sizeof(RingDescriptor);
        address += sizeof(uint16_t) * 2;
        address += sizeof(uint16_t) * size;
        address += sizeof(uint16_t);
        address = align(address, alignament);
        address += sizeof(uint16_t) * 2;
        address += sizeof(RingUsedElement) * size;
        return address;
    }

  private:
    uintptr_t address_           = 0;
    uint32_t size_               = 0;
    uint16_t last_               = 0;
    RingDescriptor *descriptors_ = nullptr;
    RingAvailable *available_    = nullptr;
    RingUsed *used_              = nullptr;
};

} // namespace QUARK::virtio
