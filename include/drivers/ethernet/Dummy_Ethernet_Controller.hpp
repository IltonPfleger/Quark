#pragma once

#include <network/NetworkDevice.hpp>

namespace QUARK {

class Dummy_Ethernet_Controller : public NetworkDevice {
  constexpr Dummy_Ethernet_Controller() = default;
  constexpr ~Dummy_Ethernet_Controller() = default;

public:
  static Dummy_Ethernet_Controller &instance() {
    static Dummy_Ethernet_Controller device;
    return device;
  }

  NetworkBuffer *alloc(size_t size) {
    uint8_t *data = new uint8_t[size];
    return new NetworkBuffer(data);
  }

  int send(NetworkBuffer *buffer) {
    size_t size = buffer->capacity();
    delete[] buffer->start();
    delete buffer;
    return size;
  }

  NetworkBuffer *receive() { return nullptr; }

  void release(NetworkBuffer *) {}
};

} // namespace QUARK
