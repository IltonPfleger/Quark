#pragma once

#include <transformer.h>

class High_Level_Controller: public Transducer<SmartData::Unit::Antigravity>, private Observer
{
    friend Responsive_SmartData<High_Level_Controller>;
    using Motion_Planning_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|14<<16|150)>>;

private:
    typedef struct _Actuation {
        float throttle;
        float brake;
        float steer;
        unsigned char hand_brake;
        unsigned char reverse;
    } Actuation; 

public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;
    static const UInt64 ACTUATION_PERIOD = 10000;

    typedef __UTIL::Observer Observer;

public:
    High_Level_Controller(const Device_Id & dev) {
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        _my_socket = new UDP_Socket(dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function

        // inputs
        _mp  = new Motion_Planning_Proxy(Motion_Planning_Proxy::Region(0, 0, 0, 100, Motion_Planning_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 30);
        db<SmartData>(TRC) << "HL-Control interests created!" << endl;
        // attach my inputs to trigger my update
        _mp->attach(this);
        _ts = new Throttle_Source(32, EXPIRY, SmartData::ADVERTISED);
        _bs = new Brake_Source(33, EXPIRY, SmartData::ADVERTISED);
        _ss = new Steer_Source(31, EXPIRY, SmartData::ADVERTISED);
        _hs = new Hand_Brake_Source(35, EXPIRY, SmartData::ADVERTISED);
        _rs = new Reverse_Source(34, EXPIRY, SmartData::ADVERTISED);

        _last_consumption = 0;
    }

    ~High_Level_Controller() { delete _my_socket; delete _ts; delete _bs; delete _ss; delete _hs; delete _rs; }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        db<SmartData>(TRC) << "HL-Control updated!" << _mp->expired() << endl;
        if (/*!_mp->expired() &&*/ _mp->when() > _last_consumption) {
            _last_consumption = _mp->now();
            transform();
        }
    }

private:
    bool transform() {
        #ifdef NO_DATA_SOURCE
            *_ts = 0;
            *_bs = 0;
            *_ss = 0;
            *_hs = 0;
            *_rs = 0;
        #else
        Motion_Planning_Proxy::Value mpv = *_mp;
        _my_socket->send(mpv, sizeof(Motion_Planning_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
    
        db<SmartData>(TRC) << "HL-Control::Awaiting response from Algorithm..." << endl;
        Actuation *a = reinterpret_cast<Actuation*>(const_cast<unsigned char*>(_my_socket->receive()));
        db<SmartData>(TRC) << "HL-Control::back from algorithm, states=" << _my_socket->size() << ",steer=" << a[0].steer << endl;
        for (UInt32 i = 0; i < (UInt32)_my_socket->size()/sizeof(Actuation); i++)
        {
            *_ts = a[0].throttle;
            *_bs = a[0].brake;
            *_ss = a[0].steer;
            *_hs = a[0].hand_brake;
            *_rs = a[0].reverse;
            usleep(ACTUATION_PERIOD);
        }
        #endif
        SmartData::Time t = _ts->now();
        db<SmartData>(LOGGER) << t << endl;
        return true;
    }

private:
    Motion_Planning_Proxy *_mp;
    Throttle_Source *_ts;
    Brake_Source *_bs;
    Steer_Source *_ss;
    Hand_Brake_Source *_hs;
    Reverse_Source *_rs;

    Value _value;
    UDP_Socket * _my_socket;
    SmartData::Time _last_consumption;
};


using High_Level_Control = Responsive_SmartData<High_Level_Controller>;
