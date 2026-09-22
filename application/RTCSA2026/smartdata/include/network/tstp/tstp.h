#pragma once

// EPOS Trustful SpaceTime Protocol Declarations

#define __tstp__ 1

#if !defined(__tstp_common_h) && defined(__tstp__)
#define __tstp_common_h

#define __nic_common_only__
#include <machine/nic.h>
#undef __nic_common_only__
#include <smartdata.h>
#include <utility/convert.h>

class TSTP : private Traits<TSTP>::NIC_Family, private SmartData, private NIC<Traits<TSTP>::NIC_Family>::Observer {
    friend class Network;
    friend class Epoch;
    friend class TSTP_PCAP_Sniffer;

  private:
    static const bool drop_expired = true;

  public:
    // Parts
    typedef Traits<TSTP>::NIC_Family NIC_Family;
    template <typename Engine, bool duty_cycling> class MAC;
    class Router;
    class Locator;
    class Timekeeper;
    class Manager;
    class Security;

    // Imports
    using typename NIC_Family::Buffer;
    using typename NIC_Family::Configuration;
    using typename NIC_Family::Metadata;
    using typename NIC_Family::Statistics;

    typedef Data_Observer<Buffer> Observer;
    typedef Data_Observed<Buffer> Observed;

    // Header
    //   typedef SmartData::Header Header;
    class Header : public SmartData::Header // Packet ID is never used inside TSTP
    {
      public:
        typedef unsigned short Packet_Id;

      public:
        Header() {}

        void identify() {
            _id             = 0;
            const char *ptr = reinterpret_cast<const char *>(this);
            for (UInt32 i = 0; i < 10; i += 2)
                _id ^= (ptr[i] << 8) | ptr[i + 1];
            _id &= 0xffff;
        }

      private:
        Packet_Id _id;
    } __attribute__((packed));

    // Packet
    // Each TSTP message is encapsulated in a single package. TSTP does not need nor supports fragmentation.
    class Packet : public Header {
      public:
        static const UInt32 MTU = NIC_Family::MTU - sizeof(Header);
        typedef unsigned char Data[MTU];

      public:
        Packet() {}

        Header *header() { return this; }

        template <typename T> T *data() { return reinterpret_cast<T *>(&_data); }

        friend Debug &operator<<(Debug &db, const Packet &p);

      private:
        unsigned char _data[MTU];
    } __attribute__((packed));

    // This field is packed first and matches the Frame Type field in the Frame Control in IEEE 802.15.4 MAC.
    // A version number above 4 renders TSTP into the reserved frame type zone and should avoid interference.
    enum Version { V0 = 4 };

  protected:
    TSTP(NIC<NIC_Family> *nic);

  public:
    ~TSTP();

    static Buffer *alloc(UInt32 size_message, UInt32 size_data = 0);
    static int send(Buffer *buf);

    // Local Space-Time (network scope, sink at center)
    static Space here();
    static Time now();
    static Time_Stamp time_stamp();

    static Time relative(const Time &t);
    static Space relative(Global_Space global);
    static Space sink() { return Space(0, 0, 0); }

    // Global Space-Time
    static Global_Space absolute(const Space &s);
    static Time absolute(const Time &t);

    // Timer-related service routines
    static const PPM &timer_accuracy() { return _nic->configuration().timer_accuracy; }
    static const Hertz &timer_frequency() { return _nic->configuration().timer_frequency; }
    static Time_Stamp us2ts(const Time &time) { return Convert::us2count<Time, Time_Stamp>(timer_frequency(), time); }
    static Time ts2us(const Time_Stamp &ts) {
        return Convert::count2us<Hertz, Time_Stamp, Time>(timer_frequency(), ts);
    }

    // TSTP clients (e.g. SmartData) are unit-based Buffer observers
    static void attach(Data_Observer<Buffer> *sd) { _clients.attach(sd); }
    static void detach(Data_Observer<Buffer> *sd) { _clients.detach(sd); }
    static bool notify(Buffer *buf) { return _clients.notify(buf); }

    friend Debug &operator<<(Debug &db, const Packet &p);

  private:
    // TSTP components are unconditional Buffer observers
    static void attach_part(Data_Observer<Buffer> *part) { _parts.attach(part); }
    static void detach_part(Data_Observer<Buffer> *part) { _parts.detach(part); }
    static bool notify_part(Buffer *buf) { return _parts.notify(buf); }

    void update(NIC_Family::Observed *obs, const Protocol &prot, Buffer *buf);

    static void marshal(Buffer *buf);

  public:
    static void init();
    static void finish();

  private:
    static Security *_security;
    static Timekeeper *_timekeeper;
    static Locator *_locator;
    static Router *_router;
    static Manager *_manager;

    static NIC<NIC_Family> *_nic;
    static TSTP *_tstp;
    static Data_Observed<Buffer> _parts;
    static Data_Observed<Buffer> _clients;
};

#endif

#if !defined(__tstp_h) && !defined(__tstp_common_only__) && defined(__tstp__)
#define __tstp_h

#include <network/tstp/locator.h>
#include <network/tstp/manager.h>
#include <network/tstp/router.h>
#include <network/tstp/security.h>
#include <network/tstp/timekeeper.h>

inline TSTP::~TSTP() {
    db<TSTP>(TRC) << "TSTP::~TSTP()" << endl;
    _nic->detach(this, 0);
    delete _security;
    delete _locator;
    delete _timekeeper;
    delete _router;
    delete _manager;
}
inline TSTP::Buffer *TSTP::alloc(UInt32 size_message, UInt32 size_data) {
    UInt32 padding = 0;
    UInt32 mac     = 0;
    if (Traits<TSTP>::use_encryption) {
        padding = sizeof(TSTP::Security::Master_Secret) - size_data % sizeof(TSTP::Security::Master_Secret);
        padding = padding == sizeof(TSTP::Security::Master_Secret) ? 0 : padding;
        mac     = size_data == 0 ? 0 : sizeof(TSTP::Security::Master_Secret); // no mac on microframe messages and other
                                                                              // subtypes not concerning data
        db<TSTP>(INF) << "TSTP::alloc()" << "message_size=" << size_message << ",data=" << size_data
                      << ",pad=" << padding << endl;
    }
    return _nic->alloc(Address::BROADCAST, PROTO_TSTP, 0, 0,
                       size_message - sizeof(SmartData::Header) + sizeof(Header) + size_data + padding + mac);
}

inline int TSTP::send(TSTP::Buffer *buf) {
    marshal(buf);

    // Packet *packet = buf->frame()->data<Packet>();
    // db<TSTP>(ERR) << "SMARTDATA SEND: " << *packet << endl;

    // db<TSTP>(ERR) << "TSTP::send(buf=" << buf << ")" << endl; marshal(buf); return _nic->send(buf);
    // db<TSTP>(ERR) << "TSTP::send(buf=" << *buf << ")" << endl;
    return _nic->send(buf);
}

inline TSTP::Space TSTP::here() { return Locator::here(); }
inline TSTP::Space TSTP::relative(TSTP::Global_Space s) { return Locator::relative(s); }
inline TSTP::Global_Space TSTP::absolute(const TSTP::Space &s) { return Locator::absolute(s); }

inline TSTP::Time TSTP::now() { return Timekeeper::now(); }
inline TSTP::Time TSTP::relative(const TSTP::Time &t) { return TSTP::_timekeeper->relative(t); }
inline TSTP::Time TSTP::absolute(const TSTP::Time &t) { return t ? TSTP::_timekeeper->relative(t) : t; }

#endif
