#pragma once
#include <transducer.h>

class Throttle_Source_Transducer: public Transducer<SmartData::Unit::Acceleration | SmartData::Unit::F32>
{
    friend Responsive_SmartData<Throttle_Source_Transducer>;

public:
    static const bool active = false; // do not trigger interruptions
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR | ACTUATOR; // transformer result

public:
    Throttle_Source_Transducer(const Device_Id & dev): _value(0) {}

    ~Throttle_Source_Transducer() {}

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return UNCERTAINTY; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {}

private:
    Value _value;
};

class Throttle_Actuator_Transducer: public Transducer<SmartData::Unit::Acceleration | SmartData::Unit::F32>
{
    friend Responsive_SmartData<Throttle_Actuator_Transducer>;
public:
    static const bool active = false; // do not trigger interruptions
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = ACTUATOR;
public:
    Throttle_Actuator_Transducer(const Device_Id & dev): _value(0), _dev(dev) {
        _my_socket = new UDP_Socket(dev+1, false);
    }

    ~Throttle_Actuator_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return UNCERTAINTY; }
    virtual void actuate(const Value & value) {
        db<SmartData>(TRC) << "New command received! v=" << value << endl;
        _value = value;
        _my_socket->send(reinterpret_cast<const unsigned char*>(&_value),4);
    }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed * obs) {}

private:
    Value _value;
    Device_Id _dev;
    UDP_Socket * _my_socket;
};

using Throttle_Source = Responsive_SmartData<Throttle_Source_Transducer>;
using Throttle_Actuator = Responsive_SmartData<Throttle_Actuator_Transducer>; // no proxy since this is the last step of the actuation (actuate itself) -- interest into the actuatio value must be issued to the source