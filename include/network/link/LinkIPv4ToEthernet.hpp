#ifndef __QUARK_IPV4_TO_ETHERNET_LINK_LAYER__
#define __QUARK_IPV4_TO_ETHERNET_LINK_LAYER__

#include <drivers/ethernet/EthernetDevice.hpp>
#include <network/NetworkAddress.hpp>
#include <network/NetworkBuffer.hpp>
#include <network/NetworkLinkLayer.hpp>
#include <network/protocols/ARP.hpp>
#include <network/protocols/IPv4.hpp>

namespace QUARK {

class LinkIPv4ToEthernet : public NetworkLinkLayer, public Observer<const NetworkBuffer *> {
    using MAC    = GenericAddress<6>;
    using IP     = GenericAddress<4>;
    using Router = ARP<EthernetDevice, IPv4>;

  public:
    LinkIPv4ToEthernet(EthernetDevice &device)
        : device_(device),
          router_(Router(device_)) {
        device_.attach(this);
    }

    ~LinkIPv4ToEthernet() { device_.detach(this); }

    NetworkBuffer *alloc(size_t length) override { return device_.alloc(length); }

    void free(NetworkBuffer *buffer) override { device_.free(buffer); }

    void update(const NetworkBuffer *buffer) override {
        auto *header = reinterpret_cast<Ethernet::Header *>(buffer->start());
        if (header->protocol() != IPv4::ProtocolValue) return;
        notify(buffer);
    }

    void bind(const NetworkAddress &address) override { router_.bind(address); }

    int send(const NetworkAddress &address, NetworkBuffer *buffer) override {
        if (IP(address) == IPv4Broadcast) return device_.send(EthernetBroadcast, Ethertype, buffer);

        MAC solved;
        if (router_.resolve(address, EthernetBroadcast, solved)) return device_.send(solved, Ethertype, buffer);

        return 0;
    }

  private:
    static constexpr Ethernet::Protocol Ethertype = IPv4::ProtocolValue;
    static constexpr MAC EthernetBroadcast        = {255, 255, 255, 255, 255, 255};
    static constexpr IP IPv4Broadcast             = {255, 255, 255, 255};

  private:
    EthernetDevice &device_;
    Router router_;
};

} // namespace QUARK

#endif
