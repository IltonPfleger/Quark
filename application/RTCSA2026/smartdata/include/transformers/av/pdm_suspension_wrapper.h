#pragma once

#include <transformer.h>

template<UInt32 U>
class Response_Source: public Transducer<U>
{
    friend Responsive_SmartData<Response_Source<U>>;

public:
    static const bool active = false; // do not trigger interruptions
    static const SmartData::Uncertainty UNCERTAINTY = SmartData::UNKNOWN;
    static const SmartData::Type TYPE = 3; // transformer result

public:
    Response_Source(const SmartData::Device_Id & dev): _value(0) {}
    ~Response_Source() {}
    virtual typename Transducer<U>::Value sense() { return _value; }
    virtual void actuate(const typename Transducer<U>::Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

private:
    typename Transducer<U>::Value _value;
};

using Front_Stiffness_Source =  Responsive_SmartData<Response_Source<SmartData::Unit::Stiffness|SmartData::Unit::F32>>;
using Rear_Stiffness_Source =   Responsive_SmartData<Response_Source<SmartData::Unit::Stiffness|SmartData::Unit::F32>>;
using Front_Damping_Source =    Responsive_SmartData<Response_Source<SmartData::Unit::Damping|SmartData::Unit::F32>>;
using Rear_Damping_Source =     Responsive_SmartData<Response_Source<SmartData::Unit::Damping|SmartData::Unit::F32>>;

class Suspension_Prediction_Transformer : public Transducer<SmartData::Unit::Suspension_Prediction>, private Observer
{
    friend Responsive_SmartData<Suspension_Prediction_Transformer>;

    using Dynamics_State_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|12<<16|1)>>; // to get velocity
    using Speed_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Speed|SmartData::Unit::F32)>>;
    using Accel_Longitudinal_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using Accel_Lateral_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using Accel_Vertical_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using Pitch_Rate_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angular_Velocity | SmartData::Unit::F32)>>;
    using Roll_Rate_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angular_Velocity | SmartData::Unit::F32)>>;
    // this two guys bellow are give in percent of vehicle actuation --> @Guilherme, this is the value you will need? 
    // If so, there is a need to calculate this by hand based on the percentages
    using Steering_Angle_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::F32)>>;
    using Throttle_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;

private:
    typedef struct _SuspensionParameters {
        float front_stiffness;
        float rear_stiffness;
        float front_damping;
        float rear_damping;
    } SuspensionParameters;

public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Suspension_Prediction_Transformer(const Device_Id & dev) {
        _my_socket = new UDP_Socket(dev+1, false);

        // Inputs
        _d  = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 16);
        _sx = new Speed_Proxy(Speed_Proxy::Region(0, 0, 0, 100, Speed_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 3);
        _sy = new Speed_Proxy(Speed_Proxy::Region(0, 0, 0, 100, Speed_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 4);
        _sz = new Speed_Proxy(Speed_Proxy::Region(0, 0, 0, 100, Speed_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 5);
        _ax  = new Accel_Longitudinal_Proxy(Accel_Longitudinal_Proxy::Region(0, 0, 0, 100, Accel_Longitudinal_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 0);
        _ay  = new Accel_Lateral_Proxy(Accel_Lateral_Proxy::Region(0, 0, 0, 100, Accel_Lateral_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 1);
        _az  = new Accel_Vertical_Proxy(Accel_Vertical_Proxy::Region(0, 0, 0, 100, Accel_Vertical_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 2);
        _pitch_rate = new Pitch_Rate_Proxy(Pitch_Rate_Proxy::Region(0, 0, 0, 100, Pitch_Rate_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 37);
        _roll_rate = new Roll_Rate_Proxy(Roll_Rate_Proxy::Region(0, 0, 0, 100, Roll_Rate_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 38);
        _steering_angle = new Steering_Angle_Proxy(Steering_Angle_Proxy::Region(0, 0, 0, 100, Steering_Angle_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 31);
        _throttle = new Throttle_Proxy(Throttle_Proxy::Region(0, 0, 0, 100, Throttle_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 32);

        // Attach inputs
        _d->attach(this);
        _sx->attach(this);
        _sy->attach(this);
        _sz->attach(this);
        _ax->attach(this);
        _ay->attach(this);
        _az->attach(this);
        _pitch_rate->attach(this);
        _roll_rate->attach(this);
        _steering_angle->attach(this);
        _throttle->attach(this);

        _fs = new Front_Stiffness_Source(48, EXPIRY, SmartData::ADVERTISED);
        _rs = new Rear_Stiffness_Source(49, EXPIRY, SmartData::ADVERTISED);
        _fd = new Front_Damping_Source(50, EXPIRY, SmartData::ADVERTISED);
        _rd = new Rear_Damping_Source(51, EXPIRY, SmartData::ADVERTISED);

        _last_consumption = 0;
    }

    ~Suspension_Prediction_Transformer() {
        delete _my_socket;

        delete _fs;
        delete _rs;
        delete _fd;
        delete _rd;

        delete _d;
        delete _sx;
        delete _sy;
        delete _sz;
        delete _ax;
        delete _ay;
        delete _az;
        delete _pitch_rate;
        delete _roll_rate;
        delete _steering_angle;
        delete _throttle;
    }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        db<SmartData>(TRC) << "Suspension_Prediction updated!" << endl;
        if (_d->when() > _last_consumption &&
            _sx->when() > _last_consumption &&
            _sy->when() > _last_consumption &&
            _sz->when() > _last_consumption &&
            _ax->when() > _last_consumption &&
            _ay->when() > _last_consumption &&
            _az->when() > _last_consumption &&
            _pitch_rate->when() > _last_consumption &&
            _roll_rate->when() > _last_consumption &&
            _steering_angle->when() > _last_consumption &&
            _throttle->when() > _last_consumption) {

            _last_consumption = _d->now(); // now is a member function

            transform();
        }
    }

private:
    bool transform() {
        db<SmartData>(TRC) << "Transform called!" << endl;

        SuspensionParameters sp;
        calculateStiffnessAndDamping(sp.front_stiffness, sp.rear_stiffness, sp.front_damping, sp.rear_damping);

        // this is only used if the function you will use to calculate is in python, otherwise everything can be done here.
        // if you want to use python, then you will need to send all the input data, i.e., 
        // Dynamics_State_Proxy::Value v = *_d; // access the proxy data
        // _my_socket->send(reinterpret_cast<unsigned char *>(&v), sizeof(Dynamics_State_Proxy::Value)); // you need to pass the address casted as a char and the size
        // _my_socket->receive();
        // _my_socket->send(reinterpret_cast<unsigned char *>(&v), sizeof(Dynamics_State_Proxy::Value)); // you need to pass the address casted as a char and the size
        // _my_socket->receive();
        // _my_socket->send(reinterpret_cast<unsigned char *>(&v), sizeof(Dynamics_State_Proxy::Value)); // you need to pass the address casted as a char and the size
        // _my_socket->receive();
        // _my_socket->send(reinterpret_cast<unsigned char *>(&v), sizeof(Dynamics_State_Proxy::Value)); // you need to pass the address casted as a char and the size
        // _my_socket->receive();
        // _my_socket->send(reinterpret_cast<unsigned char *>(&v), sizeof(Dynamics_State_Proxy::Value)); // you need to pass the address casted as a char and the size
        // _my_socket->receive();
        // _my_socket->send(reinterpret_cast<unsigned char *>(&v), sizeof(Dynamics_State_Proxy::Value)); // you need to pass the address casted as a char and the size
        // _my_socket->receive();

        // db<SmartData>(TRC) << "Suspension Prediction::Awaiting response from Algorithm..." << endl;
        // SuspensionParameters *sp = reinterpret_cast<SuspensionParameters*>(const_cast<unsigned char*>(_my_socket->receive()));

        *_fs = sp.front_stiffness;
        *_rs = sp.rear_stiffness;
        *_fd = sp.front_damping;
        *_rd = sp.rear_damping;
        db<SmartData>(TRC) << "Suspension_Prediction Sources updated!" << endl;


        //float whell_base = Traits<Vehicle_Models_Parameters>::wheel_base;

        return true;
    }

    void calculateStiffnessAndDamping(float &front_stiffness, float &rear_stiffness, float &front_damping, float &rear_damping) {
        front_stiffness = 10000.0f;
        rear_stiffness = 8000.0f;
        front_damping = 1500.0f;
        rear_damping = 1200.0f;
    }

private:
    Value _value;
    UDP_Socket *_my_socket;
    Dynamics_State_Proxy *_d;
    Speed_Proxy *_sx;
    Speed_Proxy *_sy;
    Speed_Proxy *_sz;
    Accel_Longitudinal_Proxy *_ax;
    Accel_Lateral_Proxy *_ay;
    Accel_Vertical_Proxy *_az;
    Pitch_Rate_Proxy *_pitch_rate;
    Roll_Rate_Proxy *_roll_rate;
    Steering_Angle_Proxy *_steering_angle;
    Throttle_Proxy *_throttle;

    Front_Stiffness_Source *_fs;
    Rear_Stiffness_Source *_rs;
    Front_Damping_Source *_fd;
    Rear_Damping_Source *_rd;

    SmartData::Time _last_consumption;
};

using Suspension_Predictor = Responsive_SmartData<Suspension_Prediction_Transformer>;
