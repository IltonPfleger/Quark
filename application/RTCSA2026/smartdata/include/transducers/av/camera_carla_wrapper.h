#pragma once
#include <transducer.h>

class Camera_AV_Transducer : public Transducer<SmartData::Unit::RAW_BGR|1>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<Camera_AV_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Camera_AV_Transducer(const Device_Id & dev): _value() {
        // 5001 --> (0+1) Image socket (Carla -> SmartData)
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
        _uncertainty = UNKNOWN;
        _index = 0;
        _frags_counter = 0;
        db<SmartData>(TRC) << "Camera Transducer Created" << endl;
    }

    ~Camera_AV_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        UInt32 received = _my_socket->size();
        db<SmartData>(TRC) << "Camera updated!" << endl;
        if (received < UDP_Socket::MTU) {
            if (_index+received > sizeof(Value))
                received = sizeof(Value) - _index; // bigger than max size, prune it!
            memcpy(&_value[_index], _my_socket->data(), received);
            _index = 0;
            _frags_counter = 0;
            db<SmartData>(INF) << "Last fragment of image received! size=" << received << ",t=" << _my_socket->reception() << endl;
            notify();
            db<SmartData>(INF) << "Back from notify" << endl;
        } else {
            if (_index > sizeof(Value)) // already full, something went wrong with sensor
                return;
            if (_index+received > sizeof(Value))
                received = sizeof(Value) - _index; // bigger than max size, prune it!
            memcpy(&_value[_index], _my_socket->data(), received);
            _index += received;
            _frags_counter += 1;
            db<SmartData>(INF) << "New fragment of image received! size=" << received << ",frag[" << _frags_counter <<"],t=" << _my_socket->reception() << endl;
        }
    }

private:
    Value _value;
    UInt32 _index;
    UInt32 _frags_counter;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

using Camera_AV = Responsive_SmartData<Camera_AV_Transducer>;