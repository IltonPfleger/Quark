#pragma once

#include <Traits.hpp>
#include <hypervisor/virtio/LegacyHeader.hpp>
#include <hypervisor/virtio/Queue.hpp>
#include <libraries/libc/string.h>
#include <memory/Heap.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

namespace virtio {

class Handler {
  struct Register {
    enum {
      MAGIC = 0x000,
      VERSION = 0x004,
      DEVICE_ID = 0x008,
      VENDOR_ID = 0x00c,
      DEVICE_FEATURES = 0x010,
      DEVICE_FEATURES_SELECTOR = 0x014,
      DRIVER_FEATURES = 0x020,
      DRIVER_FEATURES_SELECTOR = 0x024,
      GUEST_PAGE_SIZE = 0x028,
      QUEUE_SELECTOR = 0x030,
      QUEUE_SIZE_MAX = 0x034,
      QUEUE_SIZE = 0x038,
      QUEUE_ALIGNMENT = 0x03c,
      QUEUE_PFN = 0x040,
      QUEUE_NOTIFY = 0x050,
      INTERRUPT_STATUS = 0x060,
      INTERRUPT_ACK = 0x064,
      STATUS = 0x070,
    };
  };

public:
  Handler(uint32_t id, uint32_t features, uint32_t descriptors) {
    header_.magic = ('t' << 24) | ('r' << 16) | ('i' << 8) | 'v';
    header_.version = 1;
    header_.device_id = id;
    header_.vendor_id = 0x554d4551;
    header_.host_features = features;
    header_.queue_num_max = descriptors;
  };

  template <typename T>
  bool read(this T &&self, uintptr_t address, void *pointer, size_t length) {
    assert(length == sizeof(uint32_t));

    uint32_t *const destination = reinterpret_cast<uint32_t *>(pointer);
    const uint32_t offset = address - self.Address;

    bool valid = false;

    switch (offset) {
    case Register::MAGIC:
    case Register::VERSION:
    case Register::DEVICE_ID:
    case Register::VENDOR_ID:
    case Register::STATUS:
    case Register::DEVICE_FEATURES:
    case Register::QUEUE_SIZE_MAX:
    case Register::INTERRUPT_STATUS:
      *destination = self.header(offset);
      valid = true;
      break;
    case Register::QUEUE_PFN:
      *destination = self.pfn();
      valid = true;
      break;
    default:
      if (offset >= 0x100 && offset < 0x140) {
        valid = true;
        *destination = self.configuration(offset - 0x100);
      }
      break;
    }

    // Console::println("R: ", (void *)address, " ", (void *)*destination);

    return valid;
  }

  template <typename T>
  bool write(this T &&self, uintptr_t address, const void *pointer,
             size_t length) {
    assert(length == sizeof(uint32_t));
    uint32_t source = *reinterpret_cast<const uint32_t *>(pointer);
    const auto offset = address - self.Address;

    switch (offset) {
    case Register::GUEST_PAGE_SIZE:
    case Register::STATUS:
    case Register::QUEUE_SELECTOR:
    case Register::DEVICE_FEATURES_SELECTOR:
    case Register::DRIVER_FEATURES_SELECTOR:
    case Register::DRIVER_FEATURES:
    case Register::QUEUE_SIZE:
    case Register::QUEUE_ALIGNMENT:
      self.header(offset) = source;
      return true;
    case Register::QUEUE_PFN:
      self.pfn(source);
      return true;
    case Register::INTERRUPT_ACK:
      self.header_.interrupt_status &= ~source;
      return true;
    case Register::QUEUE_NOTIFY:
      self.notify(source);
      return true;
    default:
      return false;
    }
  }

protected:
  uint32_t &header(this auto &self, uint32_t offset) {
    return reinterpret_cast<uint32_t *>(&self.header_)[offset / 4];
  }

  uint32_t pfn(this auto &self) {
    if (self.header_.guest_page_size == 0)
      return 0;
    return self.queues_[self.header_.queue_sel].address() /
           self.header_.guest_page_size;
  }

  void pfn(this auto &self, uint32_t source) {
    uint32_t i = self.header_.queue_sel;
    uint32_t address = source * self.header_.guest_page_size;
    uint32_t length = self.header_.queue_num;
    uint32_t align = self.header_.queue_align;

    assert(self.owner_.memory().contains(
        Chunk(address, Queue::size(length, align))));

    new (&self.queues_[i]) Queue(address, length, align);

    self.header_.queue_pfn = source;
  }

  void interrupt(this auto &self) { self.header_.interrupt_status |= 0x1; }

protected:
  LegacyHeader header_;
};

} // namespace virtio

} // namespace QUARK
