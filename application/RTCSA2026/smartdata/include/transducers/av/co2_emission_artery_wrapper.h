#pragma once
#include<transducer.h>

class CO2_Emission_Transducer: public Transducer<SmartData::Unit::Mass_Flow | SmartData::Unit::D64>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<CO2_Emission_Transducer>;
public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    CO2_Emission_Transducer(const Device_Id & dev): _value(0) {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~CO2_Emission_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return UNCERTAINTY; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return _signature; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data())) / 1000000; // 4B to Value, mg/s to kg/s
        _signature = *reinterpret_cast<SmartData::Signature *>(const_cast<unsigned char *>(_my_socket->data())+sizeof(Value));
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New Fuel_Comsumption sample received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    SmartData::Signature _signature;
    UDP_Socket * _my_socket;
};

using CO2_Emission = Responsive_SmartData<CO2_Emission_Transducer>;
