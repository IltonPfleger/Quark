// #ifndef TRANSFORMERS_AV_DYNAMICS_STATE_PYTHON_WRPAYLOADER_H
// #define TRANSFORMERS_AV_DYNAMICS_STATE_PYTHON_WRPAYLOADER_H
#pragma once
#include <transformer.h>

// TODO: Add ETSI Messages
class Dynamics_State_Transformer : public Transducer<SmartData::Unit::MOTION_VECTOR_LOCAL | 12 << 16 | 1>,
                                   private Observer {
    friend Responsive_SmartData<Dynamics_State_Transformer>;
    using Acceleration_Proxy =
        Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using Rotation_Rate_Proxy =
        Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angular_Velocity | SmartData::Unit::F32)>>;
    using Orientation_Proxy =
        Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::F32)>>;
    using Longitude_Proxy =
        Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::F32)>>;
    using Latitude_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::F32)>>;
    using Altitude_Proxy =
        Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Length | SmartData::Unit::F32)>>;
    using Speed_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Speed | SmartData::Unit::F32)>>;
    using Wheel_Telemetry_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::WHEEL_TELEMETRY)>>;
    // using SmartData::Motion_Vector Motion_Vector;

  public:
    static const bool active             = true;
    static const UInt64 EXPIRY           = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE               = SENSOR;

    typedef __UTIL::Observer Observer;

  public:
    Dynamics_State_Transformer(const Device_Id &dev)
        : _value() {
// 5013 --> (11+1) (SmartData <-> Model in Python)
#ifdef NO_DATA_SOURCE
#else
        _my_socket = new UDP_Socket(dev + 1, false); // this socket is synchronous, it will only respond when we request
                                                     // --> controlled by transform function
#endif
        _lonp = new Longitude_Proxy(Longitude_Proxy::Region(0, 0, 0, 100, Longitude_Proxy::now(), INFINITE), EXPIRY, 0,
                                    SmartData::SINGLE, SmartData::ANY, 7);
        _latp = new Latitude_Proxy(Latitude_Proxy::Region(0, 0, 0, 100, Latitude_Proxy::now(), INFINITE), EXPIRY, 0,
                                   SmartData::SINGLE, SmartData::ANY, 8);
        _altp = new Altitude_Proxy(Altitude_Proxy::Region(0, 0, 0, 100, Altitude_Proxy::now(), INFINITE), EXPIRY, 0,
                                   SmartData::SINGLE, SmartData::ANY, 9);

        _yp = new Orientation_Proxy(Orientation_Proxy::Region(0, 0, 0, 100, Orientation_Proxy::now(), INFINITE), EXPIRY,
                                    0, SmartData::SINGLE, SmartData::ANY, 13);
        _pp = new Orientation_Proxy(Orientation_Proxy::Region(0, 0, 0, 100, Orientation_Proxy::now(), INFINITE), EXPIRY,
                                    0, SmartData::SINGLE, SmartData::ANY, 14);
        _rp = new Orientation_Proxy(Orientation_Proxy::Region(0, 0, 0, 100, Orientation_Proxy::now(), INFINITE), EXPIRY,
                                    0, SmartData::SINGLE, SmartData::ANY, 15);

        _yrp = new Rotation_Rate_Proxy(Rotation_Rate_Proxy::Region(0, 0, 0, 100, Rotation_Rate_Proxy::now(), INFINITE),
                                       EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 36);
        _prp = new Rotation_Rate_Proxy(Rotation_Rate_Proxy::Region(0, 0, 0, 100, Rotation_Rate_Proxy::now(), INFINITE),
                                       EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 37);
        _rrp = new Rotation_Rate_Proxy(Rotation_Rate_Proxy::Region(0, 0, 0, 100, Rotation_Rate_Proxy::now(), INFINITE),
                                       EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 38);

        _axp = new Acceleration_Proxy(Acceleration_Proxy::Region(0, 0, 0, 100, Acceleration_Proxy::now(), INFINITE),
                                      EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 0);
        _ayp = new Acceleration_Proxy(Acceleration_Proxy::Region(0, 0, 0, 100, Acceleration_Proxy::now(), INFINITE),
                                      EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 1);
        _azp = new Acceleration_Proxy(Acceleration_Proxy::Region(0, 0, 0, 100, Acceleration_Proxy::now(), INFINITE),
                                      EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 2);

        _wt_fl = new Wheel_Telemetry_Proxy(
            Wheel_Telemetry_Proxy::Region(0, 0, 0, 100, Wheel_Telemetry_Proxy::now(), INFINITE), EXPIRY, 0,
            SmartData::SINGLE, SmartData::ANY, 60);
        _wt_fr = new Wheel_Telemetry_Proxy(
            Wheel_Telemetry_Proxy::Region(0, 0, 0, 100, Wheel_Telemetry_Proxy::now(), INFINITE), EXPIRY, 0,
            SmartData::SINGLE, SmartData::ANY, 61);
        _wt_rl = new Wheel_Telemetry_Proxy(
            Wheel_Telemetry_Proxy::Region(0, 0, 0, 100, Wheel_Telemetry_Proxy::now(), INFINITE), EXPIRY, 0,
            SmartData::SINGLE, SmartData::ANY, 62);
        _wt_rr = new Wheel_Telemetry_Proxy(
            Wheel_Telemetry_Proxy::Region(0, 0, 0, 100, Wheel_Telemetry_Proxy::now(), INFINITE), EXPIRY, 0,
            SmartData::SINGLE, SmartData::ANY, 63);

        // inputs
        // speed -- this input may be available if some sensor for speed is implemented, otherwhise, this info is filled
        // by the kalman filter
        if (!Traits<Project>::speed_derived) {
            _sxp = new Speed_Proxy(Speed_Proxy::Region(0, 0, 0, 100, Speed_Proxy::now(), INFINITE), EXPIRY, 0,
                                   SmartData::SINGLE, 3);
        }

        db<SmartData>(LOGGER) << "Dynamics_State interests created!" << endl;
        _lonp->attach(this);
        _latp->attach(this);
        _altp->attach(this);

        _yp->attach(this);
        _pp->attach(this);
        _rp->attach(this);

        _yrp->attach(this);
        _prp->attach(this);
        _rrp->attach(this);

        _axp->attach(this);
        _ayp->attach(this);
        _azp->attach(this);

        if (!Traits<Project>::speed_derived) {
            _sxp->attach(this);
        }

        // attach my inputs to trigger my update
        _last_consumption = 0;
    }

    ~Dynamics_State_Transformer() {
        delete _my_socket;
        delete _lonp;
        delete _latp;
        delete _altp;
        delete _yp;
        delete _pp;
        delete _rp;
        delete _yrp;
        delete _prp;
        delete _rrp;
        delete _axp;
        if (!Traits<Project>::speed_derived) {
            delete _sxp;
        }
    }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return UNCERTAINTY; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        db<SmartData>(TRC) << "Dynamics_State updated!"
                           << " axp:" << (_axp->when() > _last_consumption)
                           << " ayp:" << (_ayp->when() > _last_consumption)
                           << " azp:" << (_azp->when() > _last_consumption)
                           << " yrp:" << (_yrp->when() > _last_consumption)
                           << " prp:" << (_prp->when() > _last_consumption)
                           << " rrp:" << (_rrp->when() > _last_consumption)
                           << " yp:" << (_yp->when() > _last_consumption)
                           << " lonp:" << (_lonp->when() > _last_consumption)
                           << " latp:" << (_latp->when() > _last_consumption)
                           << " altp:" << (_altp->when() > _last_consumption) << endl;
        if (/*!_axp->expired() &&*/ _axp->when() > _last_consumption &&
            /*!_axp->expired() &&*/ _ayp->when() > _last_consumption &&
            /*!_axp->expired() &&*/ _azp->when() > _last_consumption &&
            /*!_yrp->expired() &&*/ _yrp->when() > _last_consumption &&
            /*!_yrp->expired() &&*/ _prp->when() > _last_consumption &&
            /*!_yrp->expired() &&*/ _rrp->when() > _last_consumption &&
            /*!_yrp->expired() &&*/ _yp->when() > _last_consumption &&
            ///*!_yrp->expired() &&*/ _rp->when() > _last_consumption && // Pitch and Roll are optional
            ///*!_yrp->expired() &&*/ _pp->when() > _last_consumption &&
            /*!_lonp->expired() &&*/ _lonp->when() > _last_consumption &&
            /*!_latp->expired() &&*/ _latp->when() > _last_consumption &&
            /*!_altp->expired() &&*/ _altp->when() > _last_consumption
            // we do not wait for an update on _strp as it is updated future in the flow with a smaller update rate, so,
            // it is expected that every other SD is already updated
        ) {
            if (!Traits<Project>::speed_derived) {
                if (!(/*!_sxp->expired() &&*/ _sxp->when() > _last_consumption)) return;
            }
            _last_consumption = _lonp->now();
            if (transform()) notify();
        }
    }

  private:
    bool transform() {
#ifdef NO_DATA_SOURCE
        Motion_Vector *mv = reinterpret_cast<Motion_Vector *>(&_value);
        mv->_valid        = 0x1;
        mv->_location[0]  = 0;
        mv->_speed        = 0;
        mv->_acceleration = 0;
        mv->_id           = 0;
        mv->_uncertainty  = 0;
        mv->_obj_class    = 12;
        mv->_timestamp    = _lonp->now();
#else
        *reinterpret_cast<bool *>(&_value) = true;
        _my_socket->send(_value, sizeof(Value));
        _my_socket->receive(); // discard what i'm writting
        float f;
        f = *_lonp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Longitude_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_latp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Latitude_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_altp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Altitude_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_yp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Orientation_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_pp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Orientation_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_rp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Orientation_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_yrp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Rotation_Rate_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_prp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Rotation_Rate_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_rrp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Rotation_Rate_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_axp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Acceleration_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_ayp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Acceleration_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        f = *_azp;
        _my_socket->send(reinterpret_cast<unsigned char *>(&f), sizeof(Acceleration_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting
        Space_Time::Time t = _lonp->when();
        _my_socket->send(reinterpret_cast<unsigned char *>(&t), sizeof(Space_Time::Time));
        _my_socket->receive(); // discard what i'm writting

        db<SmartData>(INF) << "Awaiting for KF response!" << endl;
        const unsigned char *data = _my_socket->receive();
        _value                    = *reinterpret_cast<const Value *>(
            data); // must return the same _value if nothing changed, if a value is updated, it should be returned in
                                      // the respective position defined here
        for (UInt32 i = _my_socket->size(); i < sizeof(Value); i++)
            _value[i] = 0;
#endif
        return true;
    }

  private:
    Longitude_Proxy *_lonp;
    Latitude_Proxy *_latp;
    Altitude_Proxy *_altp;

    Orientation_Proxy *_yp;
    Orientation_Proxy *_pp;
    Orientation_Proxy *_rp;

    Rotation_Rate_Proxy *_yrp;
    Rotation_Rate_Proxy *_prp;
    Rotation_Rate_Proxy *_rrp;

    Acceleration_Proxy *_axp;
    Acceleration_Proxy *_ayp;
    Acceleration_Proxy *_azp;

    // Optional... just to suffice integration with datasets
    Speed_Proxy *_sxp;

    Wheel_Telemetry_Proxy *_wt_fl; // front left
    Wheel_Telemetry_Proxy *_wt_fr; // front right
    Wheel_Telemetry_Proxy *_wt_rl; // rear left
    Wheel_Telemetry_Proxy *_wt_rr; // rear right

    Value _value;
    UDP_Socket *_my_socket;
    SmartData::Time _last_consumption;
};

class Dynamics_State_Sensor : public Transducer<SmartData::Unit::MOTION_VECTOR_LOCAL | 12 << 16 | 1>,
                              private UDP_Socket::Observer {
    friend Responsive_SmartData<Dynamics_State_Sensor>;

  public:
    static const bool active             = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE               = SENSOR;

    typedef __UTIL::Observer Observer;

  public:
    Dynamics_State_Sensor(const Device_Id &dev)
        : _value() {
        _my_socket = new UDP_Socket(dev + 1);
        _my_socket->attach(this);
    }

    ~Dynamics_State_Sensor() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return UNCERTAINTY; }
    virtual void actuate(const Value &value) { _value = value; }
    virtual SmartData::Signature signature() { return _signature; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<const Value *>(_my_socket->data());
        for (UInt32 i = _my_socket->size(); i < sizeof(Value); i++)
            _value[i] = 0;
        _signature =
            *reinterpret_cast<SmartData::Signature *>(const_cast<unsigned char *>(_my_socket->data()) + sizeof(Value));
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New Dynamic received! t=" << _my_socket->reception() << endl;
    }

  private:
    Value _value;
    SmartData::Signature _signature;
    UDP_Socket *_my_socket;
};

#ifdef ARTERY_PROJECT
using Dynamics_State = Responsive_SmartData<Dynamics_State_Sensor>;
#else
using Dynamics_State = Responsive_SmartData<Dynamics_State_Transformer>;
#endif
// #endif // TRANSFORMERS_AV_DYNAMICS_STATE_PYTHON_WRPAYLOADER_H
