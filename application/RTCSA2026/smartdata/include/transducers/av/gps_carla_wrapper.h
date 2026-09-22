#pragma once
#include<transducer.h>

class GNSS_LL: public Transducer<SmartData::Unit::Angle|SmartData::Unit::F32>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<GNSS_LL>;
public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    GNSS_LL(const Device_Id & dev): _value(0) {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~GNSS_LL() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data())); // 4B to Value
        _uncertainty = *reinterpret_cast<Uncertainty *>(const_cast<unsigned char *>(_my_socket->data())+sizeof(Value));
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New GPS-location sample received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

class GNSS_A: public Transducer<SmartData::Unit::Length|SmartData::Unit::F32>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<GNSS_A>;
public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    GNSS_A(const Device_Id & dev): _value(0) {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~GNSS_A() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data())); // 4B to Value
        _uncertainty = *reinterpret_cast<Uncertainty *>(const_cast<unsigned char *>(_my_socket->data())+sizeof(Value));
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New GPS-location sample received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

using Longitude = Responsive_SmartData<GNSS_LL>;
using Latitude = Responsive_SmartData<GNSS_LL>;
using Altitude = Responsive_SmartData<GNSS_A>;

