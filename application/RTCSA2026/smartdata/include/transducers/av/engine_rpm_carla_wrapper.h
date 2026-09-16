#pragma once
#include <transducer.h>

class Engine_RPM_Reader: public Transducer<SmartData::Unit::Frequency|SmartData::Unit::F32>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<Engine_RPM_Reader>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;
    static constexpr const float FROM_RPM_TO_HZ = 1/60.0;

    typedef __UTIL::Observer Observer;

public:
    Engine_RPM_Reader(const Device_Id & dev): _value(0) {
        // 5003 --> (2+1) Image socket (Carla -> SmartData)
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Engine_RPM_Reader() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<const Value *>(_my_socket->data()) * FROM_RPM_TO_HZ;
        _uncertainty = *reinterpret_cast<Uncertainty *>(const_cast<unsigned char *>(_my_socket->data())+sizeof(Value));
        notify(); // update() SmartData
    }

private:
    Value _value;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

using Engine_RPM = Responsive_SmartData<Engine_RPM_Reader>;
