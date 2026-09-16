#pragma once
#include <transducer.h>

class RADAR_Transducer: public Transducer<SmartData::Unit::PCD_MONOCROMATIC|1>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<RADAR_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    RADAR_Transducer(const SmartData::Device_Id & dev): _value() {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
        _uncertainty = UNKNOWN;
    }

    ~RADAR_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        UInt32 received = _my_socket->size();
        if (received < UDP_Socket::MTU) {
            if (_index+received > sizeof(Value))
                received = sizeof(Value) - _index; // bigger than max size, prune it!
            memcpy(&_value[_index], _my_socket->data(), received);
            _index += received;
            for (UInt32 i = _index; i < sizeof(Value); i++) // erase remaning bytes
                _value[i] = 0;
            _index = 0;
            notify();
            db<SmartData>(TRC) << "Last fragment of RADAR received! size=" << received << " t=" << _my_socket->reception() << endl;
        } else {
            if (_index > sizeof(Value)) // already full, something went wrong with sensor
                return;
            if (_index+received > sizeof(Value))
                received = sizeof(Value) - _index; // bigger than max size, prune it!
            memcpy(&_value[_index], _my_socket->data(), received);
            _index += received;
            db<SmartData>(TRC) << "New fragment of RADAR received! size=" << received << " t=" << _my_socket->reception() << endl;
        }
    }

private:
    Value _value;
    UInt32 _index;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

using RADAR_AV = Responsive_SmartData<RADAR_Transducer>;