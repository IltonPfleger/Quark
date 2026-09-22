#pragma once

#include <transformer.h>

// TODO: Add ETSI Messages
template<bool ground_truth>
class Object_Recognition_And_Tracking_Fuser_Transformer: public Transducer<SmartData::Unit::MOTION_VECTOR_LOCAL| 3*50>, private Observer
{
    friend Responsive_SmartData<Object_Recognition_And_Tracking_Fuser_Transformer>;
    using Object_Recognition_And_Tracking_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|50)>>;
    using ETSI_CAM_Proxy =          Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_GLOBAL| 1)>>;

public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Object_Recognition_And_Tracking_Fuser_Transformer(const Device_Id & dev) : _value() {
        // 5013 --> (11+1) (SmartData <-> Model in Python)


        if (!ground_truth){
            _my_socket = new UDP_Socket(dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function

            // inputs
            _cp  = new Object_Recognition_And_Tracking_Proxy(Object_Recognition_And_Tracking_Proxy::Region(0, 0, 0, 100, Object_Recognition_And_Tracking_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 20);
            _lp  = new Object_Recognition_And_Tracking_Proxy(Object_Recognition_And_Tracking_Proxy::Region(0, 0, 0, 100, Object_Recognition_And_Tracking_Proxy::now(),     INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 21);
            if (Traits<Project>::RADAR_IS_ON) {
                _rp  = new Object_Recognition_And_Tracking_Proxy(Object_Recognition_And_Tracking_Proxy::Region(0, 0, 0, 100, Object_Recognition_And_Tracking_Proxy::now(),     INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 22);
                _rp->attach(this);
            }
            if (Traits<Project>::ETSI_CAM_IS_ON) {
                _cam  = new ETSI_CAM_Proxy(ETSI_CAM_Proxy::Region(0, 0, 0, 100, ETSI_CAM_Proxy::now(),     INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 26);
                _cam->attach(this);
            }

            _sp  = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(),     INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 16);

            db<SmartData>(TRC) << "OBJRecTrack interests created!" << endl;
            // attach my inputs to trigger my update
            _cp->attach(this);
            _lp->attach(this);
            _sp->attach(this);

            _last_consumption = 0;
            // _value[0] = 0x0; // init a semantic empty array
        }
        else
        {
            _my_socket = new UDP_Socket(dev+1); // Assyncho
            _my_socket->attach(this);
        }

    }

    ~Object_Recognition_And_Tracking_Fuser_Transformer() {

        delete _my_socket;
        if (!ground_truth){delete _cp;
        delete _lp;
        if (_rp) delete _rp;
        delete _sp; if (_cam) delete _cam;
        }
}

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {

        if (!ground_truth){
            db<SmartData>(TRC) << "ObjRecTrack updated!" << (_cp->when() > _last_consumption) << (_lp->when() > _last_consumption) << endl;
            if (/*!_cp->expired() &&*/ _cp->when() > _last_consumption &&
                /*!_lp->expired() &&*/ _lp->when() > _last_consumption &&
                /*!_rp->expired() &&*/ _sp->when() > _last_consumption) { // do not require (wait) etsi cam
                if (Traits<Project>::RADAR_IS_ON && _rp->when() > _last_consumption)
                    return;

                if (Traits<Project>::ETSI_CAM_IS_ON && _cam->when() > _last_consumption)
                    return;

                _last_consumption = _cp->now();
                if (transform())
                    notify();
            }
        }
        else
        {

            db<SmartData>(TRC) << "Rec_track_FUSER_GT::Awaiting GT from Algorithm..." << endl;
            const unsigned char* data = _my_socket->data();
            _value = *reinterpret_cast<const Value *>(data);
            db<SmartData>(TRC) << "Rec_track_FUSER_GT::DONE..." << endl;
            for(UInt32 i = _my_socket->size(); i < sizeof(Value); i++) _value[i] = 0;
            notify();
        }

    }

private:
    
    bool transform() {
        #ifdef NO_DATA_SOURCE
            Motion_Vector* mv;
            for (UInt32 i = 0; i < 100*sizeof(Motion_Vector); i+=sizeof(Motion_Vector))
            {
                mv = reinterpret_cast<Motion_Vector*>(&_value[i]);
                mv->_valid = 0x1;
                mv->_location[0] = 10;
                mv->_speed = 10;
                mv->_acceleration = 1;
                mv->_id = i;
                mv->_uncertainty = 0;
                mv->_obj_class = 3;
                mv->_timestamp = _cp->now();
            }
        #else
            Object_Recognition_And_Tracking_Proxy::Value cpv = *_cp;
            _my_socket->send(cpv, sizeof(Object_Recognition_And_Tracking_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            Object_Recognition_And_Tracking_Proxy::Value lpv = *_lp;
            _my_socket->send(lpv, sizeof(Object_Recognition_And_Tracking_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            if (Traits<Project>::RADAR_IS_ON) {
                Object_Recognition_And_Tracking_Proxy::Value rpv = *_rp;
                _my_socket->send(rpv, sizeof(Object_Recognition_And_Tracking_Proxy::Value));
                _my_socket->receive(); // discard what i'm writting
            }

            if (Traits<Project>::ETSI_CAM_IS_ON) {
                Dynamics_State_Proxy::Value state = *_sp;
                ETSI_CAM_Proxy::Value cam = *_cam;
                map_cam_to_local(state, cam);
                _my_socket->send(cam, sizeof(ETSI_CAM_Proxy::Value)); // local objects
                _my_socket->receive(); // discard what i'm writting
            }

            db<SmartData>(TRC) << "ObjRecTrack::Awaiting response from Algorithm..." << endl;
            const unsigned char* data = _my_socket->receive();
            _value = *reinterpret_cast<const Value *>(data);
            for(UInt32 i = _my_socket->size(); i < sizeof(Value); i++) _value[i] = 0;
        #endif
        return true;
    }

    void map_cam_to_local(Dynamics_State_Proxy::Value state, ETSI_CAM_Proxy::Value &cam_objects) {
        cam_objects[0] = 0x0;
    }

private:
    Object_Recognition_And_Tracking_Proxy *_cp;
    Object_Recognition_And_Tracking_Proxy *_lp;
    Object_Recognition_And_Tracking_Proxy *_rp;
    Dynamics_State_Proxy *_sp;
    ETSI_CAM_Proxy *_cam;

    Value _value;
    UDP_Socket * _my_socket;
    SmartData::Time _last_consumption;
};


using Object_Recognition_And_Tracking_Fuser = Responsive_SmartData<Object_Recognition_And_Tracking_Fuser_Transformer<false>>;
using Object_Recognition_And_Tracking_Fuser_Proxy = Interested_SmartData<SmartData::Unit::Wrap<Object_Recognition_And_Tracking_Fuser_Transformer<false>::UNIT>>;
using Object_Recognition_And_Tracking_FUSER_Ground_Truth = Responsive_SmartData<Object_Recognition_And_Tracking_Fuser_Transformer<true>>;
