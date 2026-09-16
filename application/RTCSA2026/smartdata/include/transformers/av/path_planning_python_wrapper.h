#pragma once
#include <transformer.h>

// TODO: Add ETSI Messages
class Path_Planning_Transformer: public Transducer<SmartData::Unit::MOTION_VECTOR_LOCAL|14<<16|150>, private Observer
{
    friend Responsive_SmartData<Path_Planning_Transformer>;
    using Object_Recognition_And_Tracking_Fuser_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|3*50)>>;
    using Dynamics_State_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|12<<16|1)>>;
    using Destination_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|14<<16|1)>>;
    using Camera_VVD_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Road_Condition_Class|SmartData::Unit::I32)>>;


public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Path_Planning_Transformer(const Device_Id & dev) : _value() {
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        _my_socket = new UDP_Socket(dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function

        // inputs
        _fp  = new Object_Recognition_And_Tracking_Fuser_Proxy(Object_Recognition_And_Tracking_Fuser_Proxy::Region(0, 0, 0, 100, Object_Recognition_And_Tracking_Fuser_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 23);
        _destp  = new Destination_Proxy(Destination_Proxy::Region(0, 0, 0, 100, Destination_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 41);
        _dp  = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 16);
        _vvdp  = new Camera_VVD_Proxy(Camera_VVD_Proxy::Region(0, 0, 0, 100, Camera_VVD_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 52);
        db<SmartData>(TRC) << "Path_Planning interests created!" << endl;
        // attach my inputs to trigger my update
        _fp->attach(this);
        _dp->attach(this);
        _destp->attach(this);
        _vvdp->attach(this);
        _last_consumption = 0;
        _value[0] = 0xFF;
        _value[1] = 0xFF;
        _value[2] = 0xFF;
        _value[3] = 0xFF; // init a semantic empty array
    }

    ~Path_Planning_Transformer() { delete _my_socket; delete _fp; delete _dp; delete _destp; }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        db<SmartData>(TRC) << "Path_Planning updated!" << (_fp->when() > _last_consumption) << (_dp->when() > _last_consumption) << endl;
        if (/*!_fp->expired() &&*/ _fp->when() > _last_consumption &&
             /*!_dp->expired() &&*/ _dp->when() > _last_consumption &&
             /*!_dp->expired() &&*/ _destp->when() > _last_consumption) {
            _last_consumption = _fp->now();
            if(transform())
                notify();
        }
    }

private:
    bool transform() {
        #ifdef NO_DATA_SOURCE
            Motion_Vector* mv;
            for (UInt32 i = 0; i < 5*sizeof(Motion_Vector); i+=sizeof(Motion_Vector))
            {
                mv = reinterpret_cast<Motion_Vector*>(&_value[i]);
                mv->_valid = 0x1;
                mv->_location[0] = 10;
                mv->_speed = 10;
                mv->_acceleration = 1;
                mv->_id = i;
                mv->_uncertainty = 0;
                mv->_obj_class = 14;
                mv->_timestamp = _fp->now();
            }
        #else
            _my_socket->send(_value, sizeof(Value));
            _my_socket->receive(); // discard what i'm writting
            Object_Recognition_And_Tracking_Fuser_Proxy::Value fpv = *_fp;
            _my_socket->send(fpv, sizeof(Object_Recognition_And_Tracking_Fuser_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            Destination_Proxy::Value destpv = *_destp;
            _my_socket->send(destpv, sizeof(Destination_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            Dynamics_State_Proxy::Value dpv = *_dp;
            _my_socket->send(dpv, sizeof(Dynamics_State_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting

            db<SmartData>(TRC) << "Path_Planning::Awaiting response from Algorithm..." << endl;
            const unsigned char* data = _my_socket->receive();
            _value = *reinterpret_cast<const Value *>(data);
            for(UInt32 i = _my_socket->size(); i < sizeof(Value); i++) _value[i] = 0;
        #endif
        return true;
    }

private:
    Object_Recognition_And_Tracking_Fuser_Proxy *_fp;
    Destination_Proxy *_destp;
    Dynamics_State_Proxy *_dp;
    Camera_VVD_Proxy *_vvdp;

    Value _value;
    UDP_Socket * _my_socket;
    SmartData::Time _last_consumption;
};


using Path_Planning = Responsive_SmartData<Path_Planning_Transformer>;