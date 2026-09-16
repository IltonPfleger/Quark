// EPOS Trustful Space-Time Protocol Router Implementation

// #include <system/config.h>

#define __tstp__ 1

#ifdef __tstp__

#include <main_traits.h>
#include <utility/math.h>
#include <machine/nic.h>
#include <network/tstp/tstp.h>

TSTP::Router::~Router()
{
    db<TSTP>(TRC) << "TSTP::~Router()" << endl;
    detach(this);
}

void TSTP::Router::update(Data_Observed<Buffer> * obs, Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Router::update(obs=" << obs << ",buf=" << buf << ")" << endl;

    if(buf->is_microframe) {
        db<TSTP>(WRN) << "TSTP::Router::update is microframe!" << endl;
        if(!buf->relevant)
            buf->relevant = forwarder && (buf->my_distance < buf->sender_distance);
    } else {
        Header * header = buf->frame()->data<Header>();
        // Keep Alive messages are never forwarded
        if((header->type() == CONTROL) && (header->subtype() == KEEP_ALIVE)) {
            buf->destined_to_me = false;
            db<TSTP>(INF) << "TSTP::Router::update is CONTROL OR KEEP ALIVE!" << endl;
        } else {
            Region dest = destination(buf);
            buf->destined_to_me = (((header->origin().space == here()) && ((dest.center() == Region::Center(0,0,0)) && (dest.radius() == 0)) && (dest.contains(here(), dest.t0)))
                                    || ((header->origin().space != here()) && (dest.contains(here(), dest.t0))));
            if(buf->destined_to_me)
                db<TSTP>(INF) << "TSTP::Router::update:packet is for me" << endl;

            if(forward(buf)) {
                // Forward/acknowledge the packet
                if(buf->destined_to_me)
                    return;
//                    db<TSTP>(INF) << "TSTP::Router::update:acknowledging packet" << endl;
                else
                    db<TSTP>(INF) << "TSTP::Router::update:forwarding packet" << endl;

                Buffer * send_buf = alloc(buf->size());

                // Copy frame contents
                memcpy(reinterpret_cast<unsigned char*>(send_buf->frame()), reinterpret_cast<unsigned char*>(buf->frame()), buf->size());

                // Copy buffer metadata
                send_buf->size(buf->size());
                send_buf->id = buf->id;
                send_buf->destined_to_me = buf->destined_to_me;
                send_buf->downlink = buf->downlink;
                send_buf->deadline = buf->deadline;
                send_buf->my_distance = buf->my_distance;
                send_buf->sender_distance = buf->sender_distance;
                send_buf->is_new = false;
                send_buf->is_microframe = false;
                send_buf->random_backoff_exponent = 0;

                // Calculate offset
                offset(send_buf);

                // Adjust Last Hop location
                Header * send_header = send_buf->frame()->data<Header>();
                send_header->last_hop(here());
                send_header->last_hop(now());
                send_buf->sender_distance = send_buf->my_distance;

                header->location_confidence(_locator->confidence());
                header->time_request(!Timekeeper::synchronized());

                send_buf->hint = send_buf->my_distance;

                _nic->send(send_buf);
            }
        }
    }
}


// Class Methods
void TSTP::Router::marshal(Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Router::marshal(buf=" << buf << ")" << endl;
    TSTP::Region dest = destination(buf);
    buf->downlink = (dest.center() != sink());
    buf->destined_to_me = (((buf->frame()->data<Header>()->origin().space == here()) && ((dest.center() == Region::Center(0,0,0)) && (dest.radius() == 0)) && (dest.contains(here(), dest.t0))) 
                            || ((buf->frame()->data<Header>()->origin().space != here()) && (dest.contains(here(), now()))));
    buf->hint = buf->my_distance;

    offset(buf);
}

TSTP::Region TSTP::Router::destination(Buffer * buf)
{
    Header * header = buf->frame()->data<Header>();

    switch(header->type()) {
        case INTEREST:
            return buf->frame()->data<Interest>()->region();
        case RESPONSE:
            return Region(sink(), 0, header->origin().time, header->origin().time + buf->frame()->data<Response>()->expiry());
        case COMMAND:
            return buf->frame()->data<Command>()->region();
        case CONTROL:
            switch(header->subtype()) {
                default:
                case DH_RESPONSE:
                case AUTH_REQUEST: {
                    Time t0 = header->origin().time;
                    Time t1 = Security::deadline(t0);
                    return Region(sink(), 0, t0, t1);
                }
                case DH_REQUEST: {
                    Time t0 = header->origin().time;
                    Time t1 = Security::deadline(t0);
                    return Region(buf->frame()->data<Security::DH_Request>()->destination().center(), buf->frame()->data<Security::DH_Request>()->destination().radius(), t0, t1);
                }
                case AUTH_GRANTED: {
                    Time t0 = header->origin().time;
                    Time t1 = Security::deadline(t0);
                    return Region(buf->frame()->data<Security::Auth_Granted>()->destination().center(), buf->frame()->data<Security::Auth_Granted>()->destination().radius(), t0, t1);
                }
                case REPORT: {
                    return Region(sink(), 0, header->origin().time, -1/*TODO*/);
                }
                case KEEP_ALIVE: {
                    while(true) {
                        Space fake(here().x() + (Random::random() % (RANGE / 3)), here().y() + (Random::random() % (RANGE / 3)), (here().z() + Random::random() % (RANGE / 3)));
                        if(fake != here())
                            return Region(fake, 0, 0, -1); // should never be destined_to_me
                    }
                }
                break;
                case EPOCH: {
                    return buf->frame()->data<Timekeeper::Epoch>()->destination();
                }
                case MODEL: {
                    return buf->frame()->data<Manager::Model>()->destination();
                }
            }
            break;
        default:
            db<TSTP>(WRN) << "TSTP::Locator::destination(): invalid frame type " << header->type() << endl;
            return Region(here(), 0, now() - 2, now() - 1);
    }
}

#endif
