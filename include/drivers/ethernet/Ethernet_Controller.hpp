#pragma once

#include <network/Ethernet.hpp>
#include <network/NetworkDevice.hpp>

namespace QUARK {

class Ethernet_Controller : public NetworkDevice {
public:
  using Address = Ethernet::Address;
  using Header = Ethernet::Header;
  using NetworkDevice::send;

  virtual ~Ethernet_Controller() {}

  virtual NetworkAddress address() const = 0;
  virtual void address(const NetworkAddress &) = 0;

  int send(const NetworkAddress &destination, uint16_t protocol,
           NetworkBuffer *buffer) {
    buffer->rewind(sizeof(Header));
    new (buffer->data()) Header(destination, address(), protocol);
    return send(buffer);
  };
};

} // namespace QUARK
