#ifndef __QUARK_IPV4_TO_ETHERNET_LINK_LAYER__
#define __QUARK_IPV4_TO_ETHERNET_LINK_LAYER__

#include <drivers/ethernet/Ethernet_Controller.hpp>
#include <network/NetworkAddress.hpp>
#include <network/NetworkBuffer.hpp>
#include <network/NetworkLinkLayer.hpp>
#include <network/protocols/ARP.hpp>
#include <network/protocols/IPv4.hpp>

namespace QUARK {

class LinkIPv4ToEthernet : public NetworkLinkLayer,
                           public Observer<const NetworkBuffer *> {
  using MAC = GenericAddress<6>;
  using IP = GenericAddress<4>;
  using Router = ARP<Ethernet_Controller, IPv4>;

public:
  LinkIPv4ToEthernet(Ethernet_Controller &device)
      : device_(device), router_(device_) {
    device_.attach(this);
  }

  ~LinkIPv4ToEthernet() { device_.detach(this); }

  NetworkBuffer *alloc(size_t length) override { return device_.alloc(length); }

  void update(const NetworkBuffer *buffer) override {
    auto *header = reinterpret_cast<Ethernet::Header *>(buffer->start());
    if (header->protocol() != IPv4::ProtocolValue)
      return;
    notify(buffer);
  }

  void address(const NetworkAddress &address) override {
    router_.address(address);
  }

  int send(const NetworkAddress &address, NetworkBuffer *buffer) override {
    if (IP(address) == IPv4::Broadcast)
      return device_.send(Ethernet::Broadcast, IPv4::ProtocolValue, buffer);

    MAC solved;
    if (router_.resolve(address, Ethernet::Broadcast, solved))
      return device_.send(solved, IPv4::ProtocolValue, buffer);

    return 0;
  }

private:
  Ethernet_Controller &device_;
  Router router_;
};

} // namespace QUARK

#endif
