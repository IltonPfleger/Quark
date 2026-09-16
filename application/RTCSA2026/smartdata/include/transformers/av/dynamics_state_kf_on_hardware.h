#pragma once

#include <transformer.h>

// TODO: Add ETSI Messages
class Dynamics_State_KF_On_Hardware : public Transducer<SmartData::Unit::MOTION_VECTOR_LOCAL | 12 << 16 | 1>, private Observer
{
    friend Responsive_SmartData<Dynamics_State_KF_On_Hardware>;
    using Acceleration_Proxy  = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using Rotation_Rate_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angular_Velocity | SmartData::Unit::F32)>>;
    using Orientation_Proxy   = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::F32)>>;
    using Longitude_Proxy     = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::D64)>>;
    using Latitude_Proxy      = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::D64)>>;
    using Altitude_Proxy      = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Length | SmartData::Unit::D64)>>;
    using Speed_Proxy         = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Speed | SmartData::Unit::D64)>>;
    using SmartData::Motion_Vector Motion_Vector;

public:
    static const bool active                    = true;
    static const UInt64 SECOND = 1'000'000; 
    static const UInt64 EXPIRY_IMU  = SECOND/5;
    static const UInt64 EXPIRY_GNSS = SECOND;
    static const UInt64 PERIOD_IMU  = SECOND/5;
    static const UInt64 PERIOD_GNSS = SECOND;
    static const Uncertainty UNCERTAINTY        = UNKNOWN;
    static const Type TYPE                      = SENSOR;

    typedef __UTIL::Observer Observer;

    

public:
    Dynamics_State_KF_On_Hardware(const Device_Id &dev) : _value()
    {
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        _my_socket = new UDP_Socket(dev + 1, false);  // this socket is synchronous, it will only respond when we request --> controlled by transform function

        _lonp = new Longitude_Proxy(Longitude_Proxy::Region(0, 0, 0, 100, Longitude_Proxy::now(), INFINITE), EXPIRY_GNSS, PERIOD_GNSS, SmartData::SINGLE,
                                    SmartData::ANY, 7);
        _latp = new Latitude_Proxy(Latitude_Proxy::Region(0, 0, 0, 100, Latitude_Proxy::now(), INFINITE), EXPIRY_GNSS, PERIOD_GNSS, SmartData::SINGLE,
                                   SmartData::ANY, 8);
        _altp = new Altitude_Proxy(Altitude_Proxy::Region(0, 0, 0, 100, Altitude_Proxy::now(), INFINITE), EXPIRY_GNSS, PERIOD_GNSS, SmartData::SINGLE,
                                   SmartData::ANY, 9);

        _yp = new Orientation_Proxy(Orientation_Proxy::Region(0, 0, 0, 100, Orientation_Proxy::now(), INFINITE), EXPIRY_IMU, PERIOD_IMU, SmartData::SINGLE,
                                    SmartData::ANY, 13);

        _yrp = new Rotation_Rate_Proxy(Rotation_Rate_Proxy::Region(0, 0, 0, 100, Rotation_Rate_Proxy::now(), INFINITE), EXPIRY_IMU, PERIOD_IMU,
                                       SmartData::SINGLE, SmartData::ANY, 36);
        _prp = new Rotation_Rate_Proxy(Rotation_Rate_Proxy::Region(0, 0, 0, 100, Rotation_Rate_Proxy::now(), INFINITE), EXPIRY_IMU, PERIOD_IMU,
                                       SmartData::SINGLE, SmartData::ANY, 37);
        _rrp = new Rotation_Rate_Proxy(Rotation_Rate_Proxy::Region(0, 0, 0, 100, Rotation_Rate_Proxy::now(), INFINITE), EXPIRY_IMU, PERIOD_IMU,
                                       SmartData::SINGLE, SmartData::ANY, 38);

        _axp = new Acceleration_Proxy(Acceleration_Proxy::Region(0, 0, 0, 100, Acceleration_Proxy::now(), INFINITE), EXPIRY_IMU, PERIOD_IMU, SmartData::SINGLE,
                                      SmartData::ANY, 0);
        _ayp = new Acceleration_Proxy(Acceleration_Proxy::Region(0, 0, 0, 100, Acceleration_Proxy::now(), INFINITE), EXPIRY_IMU, PERIOD_IMU, SmartData::SINGLE,
                                      SmartData::ANY, 1);
        _azp = new Acceleration_Proxy(Acceleration_Proxy::Region(0, 0, 0, 100, Acceleration_Proxy::now(), INFINITE), EXPIRY_IMU, PERIOD_IMU, SmartData::SINGLE,
                                      SmartData::ANY, 2);

        // inputs
        _sxp = new Speed_Proxy(Speed_Proxy::Region(0, 0, 0, 100, Speed_Proxy::now(), INFINITE), EXPIRY_GNSS, PERIOD_GNSS, SmartData::SINGLE, SmartData::ANY, 3);

        db<SmartData>(LOGGER) << "Dynamics_State interests created!" << endl;
        _lonp->attach(this);
        _latp->attach(this);
        _altp->attach(this);
        _yp->attach(this);
        _yrp->attach(this);
        _prp->attach(this);
        _rrp->attach(this);
        _axp->attach(this);
        _ayp->attach(this);
        _azp->attach(this);
        _sxp->attach(this);

        // attach my inputs to trigger my update
        _last_consumption = _lonp->now();
        Motion_Vector *mv = reinterpret_cast<Motion_Vector *>(&_value);
        mv->_id           = Traits<Project>::EGO_ID;
        mv->_obj_class    = 12;
        mv->_uncertainty  = 0;
        mv->_timestamp    = 0;
    }

    ~Dynamics_State_KF_On_Hardware()
    {
        delete _my_socket;
        delete _lonp;
        delete _latp;
        delete _altp;
        delete _yp;
        delete _yrp;
        delete _prp;
        delete _rrp;
        delete _axp;
        delete _ayp;
        delete _azp;
        if (!Traits<Project>::speed_derived) {
            delete _sxp;
        }
    }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs)
    {
        db<SmartData>(LOGGER) << "Dynamics_State updated!"
                              << " axp:" << (_axp->when() > _last_consumption) << " lonp:" << (_lonp->when() > _last_consumption)
                              << " latp:" << (_latp->when() > _last_consumption) << " altp:" << (_altp->when() > _last_consumption) << endl;
        if (/*!_axp->expired() &&*/ _axp->when() > _last_consumption &&
            /*!_yrp->expired() &&*/ _yrp->when() > _last_consumption &&
            /*!_yrp->expired() &&*/ _yp->when() > _last_consumption
            ///*!_yrp->expired() &&*/ _rp->when() > _last_consumption && // Pitch and Roll are optional
            ///*!_yrp->expired() &&*/ _pp->when() > _last_consumption &&
            ///*!_lonp->expired() &&*/ _lonp->when() > _last_consumption &&
            ///*!_latp->expired() &&*/ _latp->when() > _last_consumption &&
            ///*!_altp->expired() &&*/ _altp->when() > _last_consumption &&
            ///*!_sxp->expired() &&*/ _sxp->when() > _last_consumption
            // we do not wait for an update on _strp as it is updated future in the flow with a smaller update rate, so, it is expected that every other SD is
            // already updated
        ) {
            _last_consumption = _lonp->now();
            if (transform()) notify();
        }
    }

   private:
    bool transform()
    {
        Motion_Vector *mv = reinterpret_cast<Motion_Vector *>(&_value);
        mv->_valid        = true;
        mv->_location[0]  = (Int32)((*_lonp) * Traits<Project>::FLOAT_INT_PRECISION);
        mv->_location[1]  = (Int32)((*_latp) * Traits<Project>::FLOAT_INT_PRECISION);
        mv->_location[2]  = (Int32)((*_altp) * Traits<Project>::FLOAT_INT_PRECISION_M);
        mv->_speed        = *_sxp;
        mv->_heading      = *_yp;
        mv->_yaw_rate     = *_yrp;
        mv->_acceleration = *_axp;
        return true;
    }

   private:
    Longitude_Proxy *_lonp;
    Latitude_Proxy *_latp;
    Altitude_Proxy *_altp;

    Orientation_Proxy *_yp;

    Rotation_Rate_Proxy *_yrp;
    Rotation_Rate_Proxy *_prp;
    Rotation_Rate_Proxy *_rrp;

    Acceleration_Proxy *_axp;
    Acceleration_Proxy *_ayp;
    Acceleration_Proxy *_azp;

    // Optional... just to suffice integration with datasets
    Speed_Proxy *_sxp;

    Value _value;
    UDP_Socket *_my_socket;
    SmartData::Time _last_consumption;
};

using Dynamics_State_KF_On_Hardware_SD = Responsive_SmartData<Dynamics_State_KF_On_Hardware>;
