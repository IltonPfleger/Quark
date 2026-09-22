#pragma once

// EPOS Trustful SpaceTime Protocol Router Declarations

#define __tstp__ 1

#ifdef __tstp__

#include <smartdata.h>

class TSTP::Router: private SmartData, private Data_Observer<Buffer>
{

    friend class TSTP;

private:
    static const bool forwarder = true;
    static const bool drop_expired = true;

    static const UInt32 RANGE = Traits<TSTP>::RADIO_RANGE;
    typedef TSTP::Buffer Buffer;

public:
    Router();
    ~Router();

    bool synchronized();

    static Region destination(Buffer * buf);

private:
    void update(Data_Observed<Buffer> * obs, Buffer * buf);

    // Evaluates if a message must be forwarded, case in which it returns true
    bool forward(Buffer * buf) {
        if(!forwarder)
            return false;

        if(buf->my_distance >= buf->sender_distance) {
            if(!buf->destined_to_me) { // don't forward messages coming from nodes closer to the destination
                return false;
            } else {
                if(buf->frame()->data<Header>()->type() == INTEREST) { // don't forward interest messages in downlink mode
                    return false;
                }
            }
        }

        Space::Distance d = here() - buf->frame()->data<Header>()->last_hop().space;
        if(d > RANGE) // don't forward messages coming from too far away to avoid radio range asymmetry
            return false;

        Microsecond expiry = buf->deadline;

        if(expiry == INFINITE) // messages that don't expire must always be forwarded
            return true;
        else if(expiry <= now()) // care for expired message
            return !drop_expired;

        Microsecond best_case_delivery_time = (buf->my_distance + RANGE - 1) / RANGE * buf->period;
        Microsecond relative_expiry = expiry - now();
        if(best_case_delivery_time > relative_expiry) // don't forward messages that will expire before they can get to the destination
            return false;

        buf->deadline = buf->deadline - best_case_delivery_time; // make deadline local for local scheduling

        return true;
    }

    // Apply distance routing metric
    static void offset(Buffer * buf) {
        if(buf->is_new)
            buf->offset *= 1 + (buf->my_distance % RANGE);
        else
            // forward() guarantees that my_distance < sender_distance
            buf->offset *= RANGE + buf->my_distance - buf->sender_distance;
        buf->offset /= RANGE;
    }

    static void marshal(Buffer * buf);

};
#endif
