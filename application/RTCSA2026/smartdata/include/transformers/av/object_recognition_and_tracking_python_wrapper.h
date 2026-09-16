#pragma once
#include <transformer.h>

// A template-based Object_Recognition_And_Tracking_Transformer class
// Template is used to refactor the code since all Object_Recognition_And_Tracking_Transformers behave the same (input differs, so Source and dev must be provided through the template, but the transform -- send to socket 1 blob and read 1 blob -- is the same)
// Whenever the Dev of the Source SmartData is the same as the Object_Recognition_And_Tracking_Transformer, we are dealing with Transformations embedded to sensors.
// Thus, data is already transformed and this "Transformer" works as a sensor in the network, but hodling the semantics of a transformer
using Camera_AV_Proxy =       Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::RAW_BGR|1)>>;
using LIDAR_AV_Proxy =        Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::PCD_MONOCROMATIC|1)>>;
using RADAR_AV_Proxy =        Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::PCD_MONOCROMATIC|1)>>;
using Dynamics_State_Proxy =  Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|12<<16|1)>>;

template<typename Source, UInt32 source_dev>
class Object_Recognition_And_Tracking_Transformer: public Transducer<SmartData::Unit::MOTION_VECTOR_LOCAL| 50>, private Observer
{
    friend Responsive_SmartData<Object_Recognition_And_Tracking_Transformer<Camera_AV_Proxy, 17>>;
    friend Responsive_SmartData<Object_Recognition_And_Tracking_Transformer<Camera_AV_Proxy, 20>>; // Unit is Ignored -- Sensor/Transformer
    friend Responsive_SmartData<Object_Recognition_And_Tracking_Transformer<LIDAR_AV_Proxy, 18>>;
    friend Responsive_SmartData<Object_Recognition_And_Tracking_Transformer<LIDAR_AV_Proxy, 21>>; // Unit is Ignored -- Sensor/Transformer
    friend Responsive_SmartData<Object_Recognition_And_Tracking_Transformer<RADAR_AV_Proxy, 19>>;

public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;
    static const UInt32 DYNAMICS_DEV = 16;

    typedef __UTIL::Observer Observer;

public:
    Object_Recognition_And_Tracking_Transformer(const Device_Id & dev) : _value(), _dev(dev) {
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        _sp = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, DYNAMICS_DEV);
        _sp->attach(this);
        db<SmartData>(TRC) << "OBJRecTrack interest in dynamic state created!" << DYNAMICS_DEV << ",u=" << _input->UNIT << endl;

        _my_socket = new UDP_Socket(_dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function
        if (source_dev != _dev) { // A regular Transformer (relies on other sensor)
            // inputs
            _input = new Source(LIDAR_AV_Proxy::Region(0, 0, 0, 100, LIDAR_AV_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, source_dev);
            db<SmartData>(INF) << "OBJRecTrack interest created!" << source_dev << ",u=" << _input->UNIT << endl;
            // attach my inputs to trigger my update
            _input->attach(this);
        }
        _last_consumption = 0;
    }

    ~Object_Recognition_And_Tracking_Transformer() { db<SmartData>(TRC) << "~ObjRecTrack() - d=" << _dev << endl; delete _my_socket; delete _sp; if (source_dev != _dev) { delete _input;  } }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        if (source_dev != _dev) {
            db<SmartData>(TRC) << "ObjRecTrack::update() - d=" << _dev << "," << _last_consumption << _sp->when() << "," << _input->when() << endl;
            if (_input->when() > _last_consumption &&
                _sp->when() > _last_consumption) {
                transform();
                _last_consumption = _sp->now();
                notify();
            }
        } else {
            db<SmartData>(TRC) << "ObjRecTrack::update() - d=" << _dev << "," << _last_consumption << "," << _sp->when() << endl;
            if (_sp->when() > _last_consumption) {
                transform();
                notify();   
            }
        }
    }

private:
    bool transform() {
        #ifdef NO_DATA_SOURCE
            Motion_Vector* mv;
            for (UInt32 i = 0; i < 50*sizeof(Motion_Vector); i+=sizeof(Motion_Vector))
            {
                mv = reinterpret_cast<Motion_Vector*>(&_value[i]);
                mv->_valid = 0x1;
                mv->_location[0] = 10;
                mv->_speed = 10;
                mv->_acceleration = 1;
                mv->_id = i;
                mv->_uncertainty = 0;
                mv->_obj_class = 3;
                mv->_timestamp = _sp->now();
            }
        #else
            Dynamics_State_Proxy::Value sp = *_sp;
            _my_socket->send(sp, sizeof(typename Dynamics_State_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting

            if (source_dev != _dev) {
                typename Source::Value in = *_input;
                if (sizeof(typename Source::Value) > _my_socket->MTU) {
                    UInt32 mtu = _my_socket->MTU;
                    UInt32 left = sizeof(typename Source::Value);
                    UInt32 i = 0;
                    while(left > mtu) {
                        _my_socket->send(&in[i], mtu);
                        _my_socket->receive(); // discard what i'm writting
                        _my_socket->receive(); // receive ack
                        i+=mtu;
                        left-=mtu;
                        db<SmartData>(INF) << "ObjRecTrack::Send Fragments=(left=" << left << ", index="<< i/mtu << ")" << endl;
                    }
                    _my_socket->send(&in[i], left);
                    _my_socket->receive(); // discard what i'm writting
                }
            }

            db<SmartData>(TRC) << "ObjRecTrack::Awaiting response from Algorithm..." << endl;
            const unsigned char* data = _my_socket->receive();
            _value = *reinterpret_cast<const Value *>(data);
            for(UInt32 i = _my_socket->size(); i < sizeof(Value); i++) _value[i] = 0;
        #endif
        return true;
    }

private:
    Source *_input;
    Dynamics_State_Proxy *_sp;
    Value _value;
    UDP_Socket * _my_socket;
    Device_Id _dev;
    SmartData::Time _last_consumption;
};

template<typename T, UInt32 T_dev>
using Object_Recognition_And_Tracking = Responsive_SmartData<Object_Recognition_And_Tracking_Transformer<T, T_dev>>;

using Object_Recognition_And_Tracking_Camera = Object_Recognition_And_Tracking<Camera_AV_Proxy, 17>;
using Object_Recognition_And_Tracking_Camera_Transformer = Object_Recognition_And_Tracking<Camera_AV_Proxy, 20>;
using Object_Recognition_And_Tracking_LIDAR = Object_Recognition_And_Tracking<LIDAR_AV_Proxy, 18>;
using Object_Recognition_And_Tracking_LIDAR_Transformer = Object_Recognition_And_Tracking<LIDAR_AV_Proxy, 21>;
using Object_Recognition_And_Tracking_RADAR = Object_Recognition_And_Tracking<RADAR_AV_Proxy, 19>;
