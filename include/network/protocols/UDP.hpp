#pragma once

#include <network/protocols/IPv4.hpp>

namespace QUARK {

class UDP : public Observer<NetworkBuffer, const NetworkAddress &, const NetworkAddress &, uint8_t>,
            public Observed<NetworkBuffer, uint16_t, uint16_t> {

    using Handler = IPv4;

    enum { Protocol = 17 };

    struct Header {
        uint16_t source;
        uint16_t destination;
        uint16_t length;
        uint16_t checksum;
    } __attribute__((packed));

  public:
    UDP(Handler &handler, uint16_t port = 0)
        : handler_(handler),
          port_(port) {
        handler_.attach(this);
    }

    ~UDP() { handler_.detach(this); }

    NetworkBuffer *alloc(size_t length) {
        NetworkBuffer *buffer = handler_.alloc(length + sizeof(Header));
        buffer->advance(sizeof(Header));
        return buffer;
    }

    int send(const NetworkAddress &address, uint16_t port, NetworkBuffer *buffer) {
        buffer->rewind(sizeof(Header));
        size_t length       = buffer->length() - buffer->offset();
        Header *header      = buffer->data<Header *>();
        header->source      = CPU::htobe16(port_);
        header->destination = CPU::htobe16(port);
        header->length      = CPU::htobe16(length);
        header->checksum    = 0;
        return handler_.send(address, Protocol, buffer);
    }

    void update(NetworkBuffer buffer, const NetworkAddress &, const NetworkAddress &, uint8_t protocol) {
        if (protocol != Protocol) return;
        Header *header = buffer.data<Header *>();
        if (port_ && header->destination != port_) return;
        buffer.advance(sizeof(Header));
        notify(buffer, CPU::be16toh(header->destination), CPU::be16toh(header->source));
    }

  private:
    Handler &handler_;
    uint16_t port_;
};

} // namespace QUARK
