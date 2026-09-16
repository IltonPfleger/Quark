// EPOS Trustful Space-Time Protocol Implementation

#include <main_traits.h>
#include <utility/math.h>
// #include <utility/string.h>
#include <machine/nic.h>
#include <network/tstp/tstp.h>

#define __tstp__ 1

#ifdef __tstp__

TSTP::Time TSTP::Timekeeper::_reference;
TSTP::Time TSTP::Timekeeper::_skew;
volatile TSTP::Time TSTP::Timekeeper::_next_sync;
Function_Handler * TSTP::Timekeeper::_life_keeper_handler;
Alarm * TSTP::Timekeeper::_life_keeper;

const UInt32 TSTP::Timekeeper::MAX_DRIFT;


TSTP::Timekeeper::~Timekeeper()
{
    db<TSTP>(TRC) << "TSTP::~Timekeeper()" << endl;

    detach(this);
}


void TSTP::Timekeeper::update(Data_Observed<Buffer> * obs, Buffer * buf)
{
    Header * header = buf->frame()->data<Header>();
    db<TSTP>(TRC) << "TSTP::Timekeeper::update(obs=" << obs << ",buf=" << buf << ")[now=" << now() << "]" << endl;

    if(buf->is_microframe) {
        if(!synchronized())
            buf->relevant = true;
    } else {
        buf->deadline = Microsecond(Router::destination(buf).t1);
        bool closer_to_sink = buf->downlink ? ((here() - sink()) < (header->last_hop().space - sink())) : (buf->my_distance < buf->sender_distance);

        if(synchronized()) {
            if(header->time_request() && closer_to_sink) {
                db<TSTP>(INF) << "TSTP::Timekeeper::update:responding to time request" << endl;
                keep_alive();
            }
        } else {
            if(!closer_to_sink) {
                Time t0 = header->last_hop().time + NIC_TIMER_INTERRUPT_DELAY;
                Time t1 = ts2us(buf->sfdts);
                _skew = t0 - t1;
                _next_sync = now() + sync_period() / 2;
                _life_keeper->reset();

                db<TSTP>(TRC) << "TSTP::Timekeeper::update:adjusted timer offset by " << _skew << ",t0=" << t0 << ",t1=" << t1 << endl;
                db<TSTP>(INF) << "TSTP::Timekeeper::update:the time is now " << now() << " us since EPOCH (" << _reference << ")" << endl;
            }
        }
    }
}


void TSTP::Timekeeper::marshal(Buffer * buf)
{
    db<TSTP>(TRC) << "TSTP::Timekeeper::marshal(buf=" << buf << ")" << endl;

    Header * header = buf->frame()->data<Header>();

    header->origin(now());
    header->time_request(!synchronized());
    if((header->type() == CONTROL) && (header->subtype() == KEEP_ALIVE))
    	buf->deadline = now() + sync_period();
    else
        buf->deadline = Microsecond(Router::destination(buf).t1); // deadline must be set after origin time for Security messages
}


void TSTP::Timekeeper::keep_alive()
{
    db<TSTP>(TRC) << "TSTP::Timekeeper::keep_alive()" << endl;

    Buffer * buf = alloc(sizeof(Keep_Alive));
    new (buf->frame()->data<Keep_Alive>()) Keep_Alive;
    buf->frame()->data<Header>()->time_request(true);
    TSTP::send(buf);
}

#endif
