#pragma once
#include <transducer.h>

class ETSI_CAM : public Transducer<SmartData::Unit::MOTION_VECTOR_GLOBAL| 1>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<ETSI_CAM>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    ETSI_CAM(const Device_Id & dev): _value(), _signature(0) {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~ETSI_CAM() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return UNCERTAINTY; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return _signature; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<const Value *>(_my_socket->data());
        for(UInt32 i = _my_socket->size(); i < sizeof(Value); i++) _value[i] = 0;
        _signature = *reinterpret_cast<SmartData::Signature *>(const_cast<unsigned char *>(_my_socket->data())+sizeof(Value));
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New CAM received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    SmartData::Signature _signature;
    UDP_Socket * _my_socket;
};

using ETSI_CAM_Source = Responsive_SmartData<ETSI_CAM>;