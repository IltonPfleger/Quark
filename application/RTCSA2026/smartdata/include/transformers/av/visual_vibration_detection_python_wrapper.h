#pragma once
#include <transformer.h>


using Camera_AV_Proxy =       Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::RAW_BGR|1)>>;
using Dynamics_State_Proxy =  Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|12<<16|1)>>;

template<UInt32 source_dev>
class Visual_Vibration_Detection_Transformer: public Transducer<SmartData::Unit::Road_Condition_Class| SmartData::Unit::I32>, private Observer
{
    friend Responsive_SmartData<Visual_Vibration_Detection_Transformer<17>>;
    friend Responsive_SmartData<Visual_Vibration_Detection_Transformer<52>>; // Unit is Ignored -- Sensor/Transformer

public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;
    static const UInt32 DYNAMICS_DEV = 16;

    typedef __UTIL::Observer Observer;

public:
    Visual_Vibration_Detection_Transformer(const Device_Id & dev) : _value(), _dev(dev) {
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        _sp = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, DYNAMICS_DEV);
        _sp->attach(this);
        db<SmartData>(TRC) << "Visual_Vibration_Detection_Transformer Interest in dynamic state created!" << DYNAMICS_DEV << ",u=" << _input->UNIT << endl;
    
        _my_socket = new UDP_Socket(_dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function
        if (source_dev != _dev) { // A regular Transformer (relies on other sensor)
            // inputs
            _input = new Camera_AV_Proxy(Camera_AV_Proxy::Region(0, 0, 0, 100, Camera_AV_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, source_dev);
            db<SmartData>(INF) << "Visual_Vibration_Detection_Transformer Interest created!" << source_dev << ",u=" << _input->UNIT << endl;
            // attach my inputs to trigger my update
            _input->attach(this);
        }
        _last_consumption = 0;
    }

    ~Visual_Vibration_Detection_Transformer() { db<SmartData>(TRC) << "~Visual_Vibration_Detection - d=" << _dev << endl; delete _my_socket; delete _sp; if (source_dev != _dev) { delete _input;  } }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        if (source_dev != _dev) {
            db<SmartData>(TRC) << "Visual_Vibration_Detection update! - d=" << _dev << "," << _last_consumption << _sp->when() << "," << _input->when() << endl;
            if (_input->when() > _last_consumption &&
                _sp->when() > _last_consumption) {
                transform();
                _last_consumption = _sp->now();
                notify();
            }
        } else {
            db<SmartData>(TRC) << "Visual_Vibration_Detection update! - d=" << _dev << "," << _last_consumption << "," << _sp->when() << endl;
            if (_sp->when() > _last_consumption) {
                transform();
                _last_consumption = _sp->now();
                notify();   
            }
        }
    }

private:
    bool transform() {
        Dynamics_State_Proxy::Value sp = *_sp;
        _my_socket->send(sp, sizeof(typename Dynamics_State_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting

        if (source_dev != _dev) {
            typename Camera_AV_Proxy::Value in = *_input;
            if (sizeof(typename Camera_AV_Proxy::Value) > _my_socket->MTU) {
                UInt32 mtu = _my_socket->MTU;
                UInt32 left = sizeof(typename Camera_AV_Proxy::Value);
                UInt32 i = 0;
                while(left > mtu) {
                    _my_socket->send(&in[i], mtu);
                    _my_socket->receive(); // discard what i'm writting
                    _my_socket->receive(); // receive ack
                    i+=mtu;
                    left-=mtu;
                    db<SmartData>(INF) << "Visual_Vibration_Detection Send Fragments=(left=" << left << ", index="<< i/mtu << ")" << endl;
                }
                _my_socket->send(&in[i], left);
                _my_socket->receive(); // discard what i'm writting
            }
        }

        db<SmartData>(TRC) << "Visual_Vibration_Detection Awaiting response from Algorithm..." << endl;
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data()));
        return true;
    }

private:
    Camera_AV_Proxy *_input;
    Dynamics_State_Proxy *_sp;
    Value _value;
    UDP_Socket * _my_socket;
    Device_Id _dev;
    SmartData::Time _last_consumption;
};

template<UInt32 T_dev>
using Visual_Vibration_Detection = Responsive_SmartData<Visual_Vibration_Detection_Transformer<T_dev>>;
using Visual_Vibration_Detection_Camera = Visual_Vibration_Detection<17>;
using Visual_Vibration_Detection_Camera_File = Visual_Vibration_Detection<52>;
