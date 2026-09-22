#pragma once
#include <transformer.h>


using Camera_AV_Proxy =       Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::RAW_BGR|1)>>;
using Dynamics_State_Proxy =  Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|12<<16|1)>>;
using Steer_Source_Proxy =  Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::F32)>>;

template<UInt32 source_dev>
class Road_Surface_Condition_Detection_Transformer: public Transducer<SmartData::Unit::ROAD_SURFACE_CONDITION>, private Observer
{
    friend Responsive_SmartData<Road_Surface_Condition_Detection_Transformer<17>>;
    friend Responsive_SmartData<Road_Surface_Condition_Detection_Transformer<55>>; // Unit is Ignored -- Sensor/Transformer

public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;
    static const UInt32 DYNAMICS_DEV = 16;
    static const UInt32 STEER_DEV = 31;


    typedef __UTIL::Observer Observer;

public:
    Road_Surface_Condition_Detection_Transformer(const Device_Id & dev) : _value(), _dev(dev) {
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        _steer = new Steer_Source_Proxy(Steer_Source_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, STEER_DEV);
        _sp = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, DYNAMICS_DEV);
        _sp->attach(this);
        _steer->attach(this);
        db<SmartData>(TRC) << "Road_Surface_Condition_Detection_Transformer Interest in dynamic state created!" << DYNAMICS_DEV << ",u=" << _input->UNIT << endl;
    
        _my_socket = new UDP_Socket(_dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function
        if (source_dev != _dev) { // A regular Transformer (relies on other sensor)
            // inputs
            _input = new Camera_AV_Proxy(Camera_AV_Proxy::Region(0, 0, 0, 100, Camera_AV_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, source_dev);
            db<SmartData>(INF) << "Road_Surface_Condition_Detection_Transformer Interest created!" << source_dev << ",u=" << _input->UNIT << endl;
            // attach my inputs to trigger my update
            _input->attach(this);
        }
        _last_consumption = 0;
    }

    ~Road_Surface_Condition_Detection_Transformer() { db<SmartData>(TRC) << "~Road_Surface_Condition_Detection - d=" << _dev << endl; delete _my_socket; delete _sp; if (source_dev != _dev) { delete _input;  } }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        if (source_dev != _dev) {
            db<SmartData>(TRC) << "Road_Surface_Condition_Detection update! - d=" << _dev << "," << _last_consumption << _sp->when() << "," << _input->when() << endl;
            if (_input->when() > _last_consumption &&
                _sp->when() > _last_consumption &&
                _steer->when() > _last_consumption) {
                transform();
                _last_consumption = _sp->now();
                notify();
            }
        } else {
            db<SmartData>(TRC) << "Road_Surface_Condition_Detection update! - d=" << _dev << "," << _last_consumption << "," << _sp->when() << endl;
            if (_sp->when() > _last_consumption &&
                _steer->when() > _last_consumption) {
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

        Steer_Source_Proxy::Value steer = *_steer;
        _my_socket->send(reinterpret_cast<const unsigned char*>(&steer), sizeof(typename Steer_Source_Proxy::Value));
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
                    db<SmartData>(INF) << "Road_Surface_Condition_Detection Send Fragments=(left=" << left << ", index="<< i/mtu << ")" << endl;
                }
                _my_socket->send(&in[i], left);
                _my_socket->receive(); // discard what i'm writting
            }
        }

        db<SmartData>(TRC) << "Road_Surface_Condition_Detection Awaiting response from Algorithm..." << endl;
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data()));
        return true;
    }

private:
    Camera_AV_Proxy *_input;
    Dynamics_State_Proxy *_sp;
    Steer_Source_Proxy * _steer;
    Value _value;
    UDP_Socket * _my_socket;
    Device_Id _dev;
    SmartData::Time _last_consumption;
};

template<UInt32 T_dev>
using Road_Surface_Condition_Detection = Responsive_SmartData<Road_Surface_Condition_Detection_Transformer<T_dev>>;
using Road_Surface_Condition_Detection_Camera = Road_Surface_Condition_Detection<17>;
using Road_Surface_Condition_Detection_Camera_File = Road_Surface_Condition_Detection<55>;
