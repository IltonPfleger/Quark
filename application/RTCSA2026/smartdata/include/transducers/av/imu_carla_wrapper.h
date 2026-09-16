#pragma once
#include<transducer.h>

class Accelerometer: public Transducer<SmartData::Unit::Acceleration | SmartData::Unit::F32>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<Accelerometer>;
public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Accelerometer(const Device_Id & dev): _value(0) {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Accelerometer() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data())); // 4B to Value
        _uncertainty = *reinterpret_cast<Uncertainty *>(const_cast<unsigned char *>(_my_socket->data())+sizeof(Value));
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New acceleration sample received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

class Gyroscope: public Transducer<SmartData::Unit::Angular_Velocity | SmartData::Unit::F32>, private Observer
{
    friend Responsive_SmartData<Gyroscope>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Gyroscope(const Device_Id & dev): _value(0) {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Gyroscope() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data())); // 4B to Value
        _uncertainty = *reinterpret_cast<Uncertainty *>(const_cast<unsigned char *>(_my_socket->data()));
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New angular velocity sample received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

class Orientation_Transducer: public Transducer<SmartData::Unit::Angle | SmartData::Unit::F32>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<Orientation_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Orientation_Transducer(const Device_Id & dev): _value(0) {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Orientation_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data())); // 4B to Value
        _uncertainty = *reinterpret_cast<Uncertainty *>(const_cast<unsigned char *>(_my_socket->data()));
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New orientation angle sample received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

using Longitudinal_Acceleration = Responsive_SmartData<Accelerometer>;
using Lateral_Acceleration = Responsive_SmartData<Accelerometer>;
using Vertical_Acceleration = Responsive_SmartData<Accelerometer>;

using Yaw_Rate = Responsive_SmartData<Gyroscope>;
using Pitch_Rate = Responsive_SmartData<Gyroscope>;
using Roll_Rate = Responsive_SmartData<Gyroscope>;

using Yaw = Responsive_SmartData<Orientation_Transducer>;
using Pitch = Responsive_SmartData<Orientation_Transducer>;
using Roll = Responsive_SmartData<Orientation_Transducer>;