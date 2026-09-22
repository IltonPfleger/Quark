#pragma once

// EPOS Trustful SpaceTime Protocol Security Declarations

#define __tstp__ 1

#ifdef __tstp__

#include <machine/aes.h>
#define __nic_common_only__
#include <machine/nic.h>
#undef __nic_common_only__
#include <utility/poly1305.h>
#include <utility/diffie_hellman.h>
#include <utility/array.h>
#include <system/thread.h>
#include <network/tstp/tstp.h>
#include <Semaphore.hpp>


class TSTP::Security: private SmartData, private Data_Observer<Buffer>
{
    friend class TSTP;

private:
    static const bool use_encryption = Traits<TSTP>::use_encryption;
    static const UInt32 KEY_SIZE = Traits<TSTP>::KEY_SIZE;
    static const Time::Type KEY_MANAGER_PERIOD = 10 * 1000 * 1000;
    static const Time::Type KEY_EXPIRY = 1 * 60 * 1000 * 1000;
    static const Time::Type POLY_TIME_WINDOW = KEY_EXPIRY / 2;

#define _SYS

    typedef _SYS::AES<KEY_SIZE> _AES;
    typedef Diffie_Hellman<_AES> _DH;
    typedef Poly1305<_AES> _Poly1305;

public:
    typedef Array<unsigned char, KEY_SIZE> Node_Id;
    typedef Array<unsigned char, KEY_SIZE> Auth;
    typedef Array<unsigned char, KEY_SIZE> OTP;
    typedef _DH::Public_Key Public_Key;
    typedef _DH::Shared_Key Master_Secret;

    class Packed_Public_Key: public Public_Key {
    public:
        Packed_Public_Key(const Public_Key & pub): Public_Key(pub.x, pub.y, pub.z) {};
    } __attribute__((packed));

    class Peer;
    typedef Simple_List<Peer> Peers;
    class Peer
    {
    public:
        Peer(const Node_Id & id, const Region & v): _id(id), _valid(v), _el(this), _auth_time(0) {
            _aes.encrypt(_id, _id, _auth);
        }

        void valid(const Region & r) { _valid = r; }
        const Region & valid() const { return _valid; }

        bool valid_deploy(const Space & where, const Time & when) {
            return _valid.contains(where, when);
        }

        bool valid_request(const Auth & auth, const Space & where, const Time & when) {
            return !memcmp(auth, _auth, sizeof(Auth)) && _valid.contains(where, when);
        }

        const Time & authentication_time() { return _auth_time; }

        Peers::Element * link() { return &_el; }

        const Master_Secret & master_secret() const { return _master_secret; }
        void master_secret(const Master_Secret & ms) {
            _master_secret = ms;
            _auth_time = now();
        }

        const Auth & auth() const { return _auth; }
        const Node_Id & id() const { return _id; }

        friend Debug & operator<<(Debug & db, const Peer & p) {
            db << "{id=" << p._id << ",au=" << p._auth << ",v=" << p._valid << ",ms=" << p._master_secret << ",el=" << &p._el << "}";
            return db;
        }

    private:
        Node_Id _id;
        Auth _auth;
        Region _valid;
        Master_Secret _master_secret;
        Peers::Element _el;
        Time _auth_time;
    };

    class  Pending_Key;
    typedef Simple_List<Pending_Key> Pending_Keys;
    class Pending_Key
    {
    public:
        Pending_Key(const Public_Key & pk): _master_secret_calculated(false), _creation(now()), _public_key(pk), _el(this) {}

        bool expired() { return now() - _creation > KEY_EXPIRY; }

        const Master_Secret & master_secret() {
            if(_master_secret_calculated)
                return _master_secret;
            _master_secret = _dh.shared_key(_public_key);
            _master_secret_calculated = true;
            db<TSTP>(INF) << "TSTP::Security::Pending_Key: Master Secret set: " << _master_secret << endl;
            return _master_secret;
        }

        Pending_Keys::Element * link() { return &_el; };

        friend Debug & operator<<(Debug & db, const Pending_Key & p) {
            db << "{msc=" << p._master_secret_calculated << ",c=" << p._creation << ",pk=" << p._public_key << ",ms=" << p._master_secret << ",el=" << &p._el << "}";
            return db;
        }

    private:
        bool _master_secret_calculated;
        Time _creation;
        Public_Key _public_key;
        Master_Secret _master_secret;
        Pending_Keys::Element _el;
    };

    // Diffie-Hellman Request Security Bootstrap Control Message
    class DH_Request: public Control
    {
    public:
        DH_Request(const Region & d, const Public_Key & k)
        : Control(DH_REQUEST), _destination(d), _public_key(k) { }

        const Region & destination() const { return _destination; }
        void destination(const Region & d) { _destination = d; }

        Public_Key key() { return _public_key; }
        void key(const Public_Key & k) { _public_key = k; }

        friend Debug & operator<<(Debug & db, const DH_Request & m) {
            db << reinterpret_cast<const Control &>(m) << ",d=" << m._destination << ",k=" << m._public_key;
            return db;
        }

    private:
        Region _destination;
        Packed_Public_Key _public_key;
    } __attribute__((packed));

    // Diffie-Hellman Response Security Bootstrap Control Message
    class DH_Response: public Control
    {
    public:
        DH_Response(const Public_Key & k)
        : Control(DH_RESPONSE), _public_key(k) {}

        Public_Key key() { return _public_key; }
        void key(const Public_Key & k) { _public_key = k; }

        friend Debug & operator<<(Debug & db, const DH_Response & m) {
            db << reinterpret_cast<const Control &>(m) << ",k=" << m._public_key;
            return db;
        }

    private:
        Packed_Public_Key _public_key;
    } __attribute__((packed));

    // Authentication Request Security Bootstrap Control Message
    class Auth_Request: public Control
    {
    public:
        Auth_Request(const Auth & a, const OTP & o)
        : Control(AUTH_REQUEST), _auth(a), _otp(o) { }

        const Auth & auth() const { return _auth; }
        void auth(const Auth & a) { _auth = a; }

        const OTP & otp() const { return _otp; }
        void otp(const OTP & o) { _otp = o; }

        friend Debug & operator<<(Debug & db, const Auth_Request & m) {
            db << reinterpret_cast<const Control &>(m) << ",a=" << m._auth << ",o=" << m._otp;
            return db;
        }

    private:
        Auth _auth;
        OTP _otp;
    } __attribute__((packed));

    // Authentication Granted Security Bootstrap Control Message
    class Auth_Granted: public Control
    {
    public:
        Auth_Granted(const Region & d, const Auth & a)
        : Control(AUTH_GRANTED), _destination(d), _auth(a) {}

        const Region & destination() const { return _destination; }
        void destination(const Region & d) { _destination = d; }

        const Auth & auth() const { return _auth; }
        void auth(const Auth & a) { _auth = a; }

        friend Debug & operator<<(Debug & db, const Auth_Granted & m) {
            db << reinterpret_cast<const Control &>(m) << ",d=" << m._destination << ",a=" << m._auth;
            return db;
        }

    private:
        Region _destination;
        Auth _auth;
     } __attribute__((packed));

    // Report Control Message
    class Report: public Control
    {
    public:
        Report()
        : Control(REPORT) {}

        friend Debug & operator<<(Debug & db, const Report & r) {
//            db << reinterpret_cast<const Control &>(r) << ",u=" << r._unit << ",e=" << r._error << ",r=" << r._epoch_request;
            return db;
        }
    } __attribute__((packed));

public:
    Security();
    ~Security();

    static void add_peer(const unsigned char * peer_id, UInt32 id_len, const Region & valid_region) {
        Node_Id id(peer_id, id_len);
        Peer * peer = new /*(SYSTEM)*/ Peer(id, valid_region);
        _pending_peers.insert(peer->link());
        if(!_key_manager)
            _key_manager = new /*(SYSTEM)*/ Thread(&key_manager);
    }

    static Time deadline(const Time & origin) {
        return origin + Math::min(KEY_MANAGER_PERIOD, KEY_EXPIRY) / 2;
    }

private:
    void update(Data_Observed<Buffer> * obs, Buffer * buf);

    static void marshal(Buffer * buf);

    static void pack(unsigned char * msg, const Peer * peer, UInt32 size);
    static bool unpack(const Peer * peer, unsigned char * msg, const unsigned char * mac, Time reception_time, UInt32 size);

    // TODO: remove?
    static void encrypt(const unsigned char * msg, const Peer * peer, unsigned char * out) {
        OTP key = otp(peer->master_secret(), peer->id());
        _aes.encrypt(msg, key, out);
    }
    static OTP otp(const Master_Secret & master_secret, const Node_Id & id);
    static bool verify_auth_request(const Master_Secret & master_secret, const Node_Id & id, const OTP & otp);
    static void* key_manager(void*);

private:
    static QUARK::Semaphore _mtx;

    static Node_Id _id;
    static Auth _auth;

    static Thread * _key_manager;
    static Pending_Keys _pending_keys;
    static volatile bool _peers_lock;
    static Peers _pending_peers;
    static Peers _trusted_peers;
    static UInt32 _dh_requests_open;

    static _AES _aes;
    static _AES & _cipher;
    static _DH _dh;
};

#endif
