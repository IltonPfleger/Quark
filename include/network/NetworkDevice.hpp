#ifndef __QUARK_NETWORK_DEVICE__
#define __QUARK_NETWORK_DEVICE__

#include <network/NetworkBuffer.hpp>
#include <utility/Observer.hpp>

namespace QUARK {

class NetworkDevice : public Observed<const NetworkBuffer *> {
  public:
    using Observed = QUARK::Observed<const NetworkBuffer *>;
    using Observer = QUARK::Observer<const NetworkBuffer *>;
    using Observed::notify;

  public:
    virtual ~NetworkDevice()              = default;
    virtual int send(NetworkBuffer *)     = 0;
    virtual NetworkBuffer *alloc(size_t)  = 0;
    virtual NetworkBuffer *receive()      = 0;
    virtual void release(NetworkBuffer *) = 0;
};

} // namespace QUARK

#endif
