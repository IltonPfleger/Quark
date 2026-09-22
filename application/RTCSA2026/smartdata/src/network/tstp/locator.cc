// EPOS Trustful Space-Time Protocol Locator Implementation

#define __tstp__ 1

#ifdef __tstp__

#include <main_traits.h>
#include <network/tstp/tstp.h>

// Class attributes
TSTP::Global_Space TSTP::Locator::_reference;
TSTP::Locator::Engine TSTP::Locator::_engine;

// Methods
TSTP::Locator::~Locator()
{
    db<TSTP>(TRC) << "TSTP::~Locator()" << endl;

    detach(this);
}

void TSTP::Locator::update(Data_Observed<Buffer> * obs, Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Locator::update(obs=" << obs << ",buf=" << buf << ")" << endl;

    if(buf->is_microframe) {
        buf->sender_distance = buf->hint; // this would fit better in the Router, but Timekeeper uses this info
        if(!_engine.synchronized())
            buf->relevant = true;
        else if(!buf->downlink)
            buf->my_distance = here() - sink();
    } else {
        Header * header = buf->frame()->data<Header>();
        Space dst = Space(Router::destination(buf).center());
        buf->sender_distance = header->last_hop().space - dst;
        _engine.learn(header->last_hop(), header->location_confidence(), buf->rssi);

        buf->downlink = (dst != sink()); // this would fit better in the Router, but Timekeeper uses this info
        buf->my_distance = here() - dst;
        // Respond to Keep Alive if sender is low on location confidence
        if(_engine.synchronized() && (header->type() == CONTROL) && (header->subtype() == KEEP_ALIVE) && !_engine.neigbor_synchronized(header->location_confidence()))
            Timekeeper::keep_alive();
    }
}

// Class Methods
void TSTP::Locator::marshal(Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Locator::marshal(buf=" << buf << ")" << endl;
    Space dst = Space(Router::destination(buf).center());
    buf->my_distance = here() - dst;
    if(buf->is_new)
        buf->sender_distance = buf->my_distance;

    buf->downlink = dst != sink(); // This would fit better in the Router, but Timekeeper uses this info
    buf->frame()->data<Header>()->location_confidence(Locator::confidence());

    buf->frame()->data<Header>()->origin(here());
    buf->frame()->data<Header>()->origin(now());
    buf->frame()->data<Header>()->last_hop(here());
    buf->frame()->data<Header>()->last_hop(now());
}

#endif
