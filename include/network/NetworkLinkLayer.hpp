#ifndef __QUARK_NETWORK_LINK_LAYER__
#define __QUARK_NETWORK_LINK_LAYER__

#include <network/NetworkAddress.hpp>
#include <network/NetworkBuffer.hpp>
#include <utility/Observer.hpp>

namespace QUARK {

class NetworkLinkLayer : public Observed<const NetworkBuffer *> {
  public:
    virtual ~NetworkLinkLayer()                               = default;
    virtual void bind(const NetworkAddress &)                 = 0;
    virtual NetworkBuffer *alloc(size_t)                      = 0;
    virtual int send(const NetworkAddress &, NetworkBuffer *) = 0;
};

} // namespace QUARK

#endif
