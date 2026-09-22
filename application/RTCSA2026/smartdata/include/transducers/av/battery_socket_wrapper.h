#pragma once
#include <transducer.h>

class Battery_Charge_Reader: public Transducer<SmartData::Unit::Percent| SmartData::Unit::F32>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<Battery_Charge_Reader>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Battery_Charge_Reader(const Device_Id & dev): _value(0) {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Battery_Charge_Reader() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        db<SmartData>(TRC) << "New Battery_Charge_Read received!" << endl;
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data())); // 4B to Value        
        _uncertainty = *reinterpret_cast<Uncertainty *>(const_cast<unsigned char *>(_my_socket->data())+sizeof(Value));
        notify(); // update() SmartData
    }

private:
    Value _value;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

using Battery_Charge = Responsive_SmartData<Battery_Charge_Reader>;
