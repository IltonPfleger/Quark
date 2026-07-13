#pragma once

// EPOS Network Interface Mediator Common Package

#include <architecture/cpu.h>
#include <architecture/tsc.h>
#include <libraries/libc/string.h>
#include <libraries/libc/stdlib.h>
#include <assert.h>

#undef NULL

class NIC_Common
{
protected:
    NIC_Common() {}

public:
    // NIC physical address (e.g. MAC)
    template<UInt32 LENGTH>
    class Address
    {
    public:
        enum Null { NULL = 0 };
        enum Broadcast { BROADCAST = 255 };

    public:
        Address() {}

        Address(const Null &) {
            for(UInt32 i = 0; i < LENGTH; i++)
                _address[i] =  NULL;
        }

        Address(const Broadcast &) {
            for(UInt32 i = 0; i < LENGTH; i++)
                _address[i] = BROADCAST;
        }

        Address(const char * str) { // String formated as A.B.C.D or A:B:C:D:E:F
            static const char sep = (LENGTH == 4) ? '.' : ':';
            char * token = strchr(str, sep);
            for(UInt32 i = 0; i < LENGTH; i++, ++token, str = token, token = strchr(str, sep))
                _address[i] = atol(str);
        }

        Address(UInt32 a) {
            assert(LENGTH == sizeof(long));
            a = CPU::_htonl(a);
            memcpy(reinterpret_cast<unsigned char*>(this), reinterpret_cast<unsigned char*>(&a), sizeof(long));
        }

        Address operator=(const Address & a) {
            for(UInt32 i = 0; i < LENGTH; ++i)
                _address[i] = a._address[i];
            return a;
        }
        Address operator=(const Address & a) volatile {
            for(UInt32 i = 0; i < LENGTH; ++i)
                _address[i] = a._address[i];
            return a;
        }

        operator bool() const {
            for(UInt32 i = 0; i < LENGTH; ++i) {
                if(_address[i])
                    return true;
            }
            return false;
        }
        operator bool() const volatile {
            for(UInt32 i = 0; i < LENGTH; ++i) {
                if(_address[i])
                    return true;
            }
            return false;
        }

        bool operator==(const Address & a) const {
            for(UInt32 i = 0; i < LENGTH; ++i) {
                if(_address[i] != a._address[i])
                    return false;
            }
            return true;
        }

        bool operator!=(const Address & a) const {
            for(UInt32 i = 0; i < LENGTH; ++i) {
                if(_address[i] != a._address[i])
                    return true;
            }
            return false;
        }

        Address operator&(const Address & a) const {
            Address ret;
            for(UInt32 i = 0; i < LENGTH; ++i)
                ret[i] = _address[i] & a._address[i];
            return ret;
        }

        Address operator|(const Address & a) const {
            Address ret;
            for(UInt32 i = 0; i < LENGTH; ++i)
                ret[i] = _address[i] | a._address[i];
            return ret;
        }

        Address operator~() const {
            Address ret;
            for(UInt32 i = 0; i < LENGTH; ++i)
                ret[i] = ~_address[i];
            return ret;
        }

        UInt32 operator%(UInt32 i) const {
            return _address[LENGTH - 1] % i;
        }

        unsigned char & operator[](const size_t i) { return _address[i]; }
        const unsigned char & operator[](const size_t i) const { return _address[i]; }

        friend OStream & operator<<(OStream & db, const Address & a) {
            db << hex;
            for(UInt32 i = 0; i < LENGTH; i++) {
                db << a._address[i];
                if(i < LENGTH - 1)
                    db << ((LENGTH == 4) ? "." : ":");
            }
            db << dec;
            return db;
        }

    private:
        unsigned char _address[LENGTH];
    } __attribute__((packed));

    // NIC protocol id
    typedef unsigned short Protocol;

    // NIC CRCs
    typedef unsigned short CRC16;
    typedef UInt32 CRC32;

    // Configuration parameters
    struct Configuration
    {
        typedef UInt32 Selector;
        enum : Selector {
            ADDRESS     = 1 << 0,     // MAC address (all NICs, r/w)
            CHANNEL     = 1 << 1,     // current channel (wireless only, , r/w)
            POWER       = 1 << 2,     // current transmission power (wireless only, r/w)
            PERIOD      = 1 << 3,     // MAC period (time-synchronous MACs only, r/w)
            TIMER       = 1 << 4,     // timer parameters (time-synchronous MACs only, r/w)
            TIME_STAMP  = 1UL << 31,  // time stamp the configuration (always updated, also when reading, r/o)
            ALL         = 0xffffffff,
        };

        Selector selector;
        Int64 parameter; // a generic parameter used for reconfiguration operations
    };

    // NIC statistics
    struct Statistics
    {
        typedef UInt32 Count;

        Statistics(): rx_packets(0), tx_packets(0), rx_bytes(0), tx_bytes(0) {}

        Count rx_packets;
        Count tx_packets;
        Count rx_bytes;
        Count tx_bytes;
    };

    // Buffer Metadata added to frames by higher-level protocols
    struct Dummy_Metadata
    {
        // Traits
        static const bool collect_sfdts = false;
        static const bool collect_rssi = false;

        // Data (null; union just to compile inactive protocols)
        union {
            int rssi;
            TSC::Time_Stamp sfdts;
            Microsecond period;
            Microsecond deadline;
            UInt32 id;
            UInt64 offset;
            bool destined_to_me;
            bool downlink;
            UInt32 my_distance;
            UInt32 sender_distance;
            bool is_new;
            bool is_microframe;
            bool relevant;
            bool trusted;
            bool freed;
            UInt32 random_backoff_exponent;
            UInt32 microframe_count;
            int hint;
            UInt32 times_txed;
        };

        Dummy_Metadata() {};

        friend OStream & operator<<(OStream & db, const Dummy_Metadata & m) {
        	db << "{no metadata}";
        	return db;
        }
    };

    struct TSTP_Metadata
    {
        // Traits
        static const bool collect_sfdts = true;
        static const bool collect_rssi = true;

        int rssi;                             // Received Signal Strength Indicator
        TSC::Time_Stamp sfdts;                // Time stamp of start of frame delimiter reception
        Microsecond period;                   // NIC's MAC current period (in us)
    	Microsecond deadline;                 // Time until when this message must arrive at the final destination
    	UInt32 id;                      // Message identifier
    	UInt64 offset;            // MAC contention offset
    	bool destined_to_me;                  // Whether this node is the final destination for this message
    	bool downlink;                        // Message direction (downlink == from sink to sensor)
    	UInt32 my_distance;             // This node's distance to the message's final destination
    	UInt32 sender_distance;         // Last hop's distance to the message's final destination
    	bool is_new;                          // Whether this message was just created by this node
    	bool is_microframe;                   // Whether this message is a Microframe
    	bool relevant;                        // Whether any component is interested in this message
    	bool trusted;                         // If true, this message was successfully verified by the Security Manager
    	bool freed;                           // If true, the MAC will not free this buffer
    	UInt32 random_backoff_exponent; // Exponential backoff used by the MAC to avoid permanent interference
    	UInt32 microframe_count;        // Number of Microframes left until data
    	int hint;                             // Inserted in the Hint Microframe field
        UInt32 times_txed;              // Number of times the MAC transmited this buffer

        friend OStream & operator<<(OStream & db, const TSTP_Metadata & m) {
            db << "{rssi=" << m.rssi << ",period=" << m.period << ",sfdts=" << m.sfdts << ",id=" << m.id
               << ",offset=" << m.offset << ",destined_to_me=" << m.destined_to_me << ",downlink=" << m.downlink << ",deadline=" << m.deadline
               << ",my_distance=" << m.my_distance << ",sender_distance=" << m.sender_distance << ",is_new=" << m.is_new << ",is_microframe=" << m.is_microframe
               << ",relevant=" << m.relevant << ",trusted=" << m.trusted << ",freed=" << m.freed << ",hint=" << m.hint
               << "}";
            return db;
        }
    };

    private:
        static void init();
};

#include <network/ethernet.h>
#include <network/only_data_udp_wrapper.h>

// Polymorphic NIC base class
template<typename Family>
class NIC: public Family, public Family::Observed
{
public:
    using typename Family::Address;
    using typename Family::Protocol;
    using typename Family::Frame;
    using typename Family::Metadata;
    using typename Family::Buffer;
    using typename Family::Configuration;
    using typename Family::Statistics;
    using typename Family::Observed;
    using Family::MTU;

protected:
    NIC(UInt32 unit = 0) {}

public:
    virtual ~NIC() {}

    virtual int send(const Address & dst, const Protocol & prot, const void * data, UInt32 size) = 0;
    virtual int receive(Address * src, Protocol * prot, void * data, UInt32 size) = 0;

    virtual Buffer * alloc(const Address & dst, const Protocol & prot, UInt32 once, UInt32 always, UInt32 payload) = 0;
    virtual int send(Buffer * buf) = 0;
    virtual bool drop(Buffer * buf) { return false; } // after send, while still in the working queues, not supported by many NICs
    virtual void free(Buffer * buf) = 0; // to be called by observers after handling notifications from the NIC

    virtual const Address & address() = 0;
    virtual void address(const Address &) = 0;

    virtual bool reconfigure(const Configuration * c = 0) = 0; // pass null to reset
    virtual const Configuration & configuration() = 0;

    virtual const Statistics & statistics() = 0;
};

#if defined (__NIC_H) && !defined(__nic_common_only__)
#include __NIC_H
#endif
