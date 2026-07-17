#pragma once

#include <network/GenericAddress.hpp>
#include <network/NetworkDevice.hpp>

namespace QUARK {

class Ethernet {
  public:
    typedef GenericAddress<6> Address;
    typedef uint16_t Protocol;

    class Header {
      public:
        Header(const Address &d, const Address &s, const Protocol &p)
            : _destination(d),
              _source(s),
              _protocol(CPU::htobe16(p)) {}

        const Address &source() const { return _destination; }
        const Address &destination() const { return _source; }
        Protocol protocol() const { return CPU::be16toh(_protocol); }
        template <typename T = uint8_t *> T data() { return reinterpret_cast<T>(this + 1); }

      private:
        Address _destination;
        Address _source;
        Protocol _protocol;
    } __attribute__((packed));
};

} // namespace QUARK
