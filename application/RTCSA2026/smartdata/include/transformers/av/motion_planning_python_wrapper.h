#pragma once

#include <transformer.h>

class Motion_Planning_Transformer: public Transducer<SmartData::Unit::MOTION_VECTOR_LOCAL|14<<16| 150>, private Observer
{
    friend Responsive_SmartData<Motion_Planning_Transformer>;
    using Object_Recognition_And_Tracking_Fuser_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|3*50)>>;
    using Path_Planning_Proxy =  Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|14<<16|150)>>;
    using Dynamics_State_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|12<<16|1)>>;
    using Drag_Proxy =           Interested_SmartData<SmartData::Unit::Wrap<SmartData::Unit::Force|SmartData::Unit::F32>>;
    using Gear_Proxy =           Interested_SmartData<SmartData::Unit::Wrap<SmartData::Unit::Counter|SmartData::Unit::I32>>;
    using Engine_RPM_Proxy =     Interested_SmartData<SmartData::Unit::Wrap<SmartData::Unit::Frequency|SmartData::Unit::F32>>;
    using Orientation_Proxy =    Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle|SmartData::Unit::F32)>>;
    using Mass_Proxy =           Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Mass|SmartData::Unit::F32)>>;
    using Odometer_Proxy =       Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Length|SmartData::Unit::F32)>>;
    using IMU_ACC_X_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using IMU_ACC_Y_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using IMU_ACC_Z_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using Throttle_Actuator_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using Brake_Actuator_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Acceleration | SmartData::Unit::F32)>>;
    using Steer_Actuator_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::F32)>>;
    using Pitch_Rate_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angular_Velocity | SmartData::Unit::F32)>>;
    


public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Motion_Planning_Transformer(const Device_Id & dev) : _value() {
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        _my_socket = new UDP_Socket(dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function

        // inputs
        _fp  = new Object_Recognition_And_Tracking_Fuser_Proxy(Object_Recognition_And_Tracking_Fuser_Proxy::Region(0, 0, 0, 100, Object_Recognition_And_Tracking_Fuser_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 23);
        _pp  = new Path_Planning_Proxy(Path_Planning_Proxy::Region(0, 0, 0, 100, Path_Planning_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 29);
        _dp  = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 16);
        _drag_p = new Drag_Proxy(Drag_Proxy::Region(0, 0, 0, 100, Drag_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 6);
        _gear_p = new Gear_Proxy(Gear_Proxy::Region(0, 0, 0, 100, Gear_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 10);
        _erpm_p  = new Engine_RPM_Proxy(Engine_RPM_Proxy::Region(0, 0, 0, 100, Engine_RPM_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 11);
        _pitch_p = new Orientation_Proxy(Orientation_Proxy::Region(0, 0, 0, 100, Orientation_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 14);
        _mass_p = new Mass_Proxy(Mass_Proxy::Region(0, 0, 0, 100, Mass_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 53);
        _odometer_p = new Odometer_Proxy(Odometer_Proxy::Region(0, 0, 0, 100, Odometer_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 54);
        _ax  = new IMU_ACC_X_Proxy(IMU_ACC_X_Proxy::Region(0, 0, 0, 100, IMU_ACC_X_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 0);
        _ay  = new IMU_ACC_Y_Proxy(IMU_ACC_Y_Proxy::Region(0, 0, 0, 100, IMU_ACC_Y_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 1);
        _az  = new IMU_ACC_Z_Proxy(IMU_ACC_Z_Proxy::Region(0, 0, 0, 100, IMU_ACC_Z_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 2);
        _sp = new Steer_Actuator_Proxy(Steer_Actuator_Proxy::Region(0, 0, 0, 100, Steer_Actuator_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 31);
        _tp = new Throttle_Actuator_Proxy(Throttle_Actuator_Proxy::Region(0, 0, 0, 100, Throttle_Actuator_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 32);
        _bp = new Brake_Actuator_Proxy(Brake_Actuator_Proxy::Region(0, 0, 0, 100, Brake_Actuator_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 33);
        _pitch_rate = new Pitch_Rate_Proxy(Pitch_Rate_Proxy::Region(0, 0, 0, 100, Pitch_Rate_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 37);

        db<SmartData>(TRC) << "Motion_Planning interests created!" << endl;
        // attach my inputs to trigger my update
        _fp->attach(this);
        _pp->attach(this);
        _dp->attach(this);
        _drag_p->attach(this);
        _gear_p->attach(this);
        _erpm_p->attach(this);
        _pitch_p->attach(this);
        _mass_p->attach(this);
        _odometer_p->attach(this);
        _ax->attach(this);
        _ay->attach(this);
        _az->attach(this);
        _pitch_rate->attach(this);        
        _tp->attach(this);
        _bp->attach(this);
        _sp->attach(this);


        _last_consumption = _pp->now();
        _value[0] = 0xFF;
        _value[1] = 0xFF;
        _value[2] = 0xFF;
        _value[3] = 0xFF; // init a semantic empty array
    }

    ~Motion_Planning_Transformer() 
    { 
        delete _my_socket; 
        delete _fp; 
        delete _pp; 
        delete _dp; 
        delete _drag_p; 
        delete _gear_p; 
        delete _erpm_p; 
        delete _pitch_p;
        delete _ax;
        delete _ay;
        delete _az;
        delete _pitch_rate;
        delete _tp; 
        delete _bp; 
        delete _sp;
    }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        db<SmartData>(TRC) << "Motion_Planning updated!" << (_fp->when() > _last_consumption) <<  "," << (_pp->when() > _last_consumption) << "," << (_dp->when() > _last_consumption) << endl;
        if (/*!_fp->expired() &&*/ _fp->when() > _last_consumption &&
            /*!_pp->expired() &&*/ _pp->when() > _last_consumption &&
            /*!_dp->expired() &&*/ _dp->when() > _last_consumption) {
            _last_consumption = _pp->now();
            if (transform())
                notify();
        }
    }

private:
    bool transform() {
        #ifdef NO_DATA_SOURCE
            Motion_Vector* mv;
            for (UInt32 i = 0; i < 2*sizeof(Motion_Vector); i+=sizeof(Motion_Vector))
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
            Path_Planning_Proxy::Value ppv = *_pp;
            _my_socket->send(ppv, sizeof(Path_Planning_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            Dynamics_State_Proxy::Value dpv = *_dp;
            _my_socket->send(dpv, sizeof(Dynamics_State_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            Drag_Proxy::Value dragpv = *_drag_p;
            _my_socket->send(reinterpret_cast<unsigned char*>(&dragpv), sizeof(Drag_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            Gear_Proxy::Value gearpv = *_gear_p;
            _my_socket->send(reinterpret_cast<unsigned char*>(&gearpv), 4); // FIXME: instead of 4 it should use something like sizeof(Gear_Proxy::Value)
            _my_socket->receive(); // discard what i'm writting
            Engine_RPM_Proxy::Value erpmv = *_erpm_p;
            _my_socket->send(reinterpret_cast<unsigned char*>(&erpmv), sizeof(Engine_RPM_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            Orientation_Proxy::Value pitchv = *_pitch_p;
            _my_socket->send(reinterpret_cast<unsigned char*>(&pitchv), sizeof(Orientation_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting
            Pitch_Rate_Proxy::Value prv = *_pitch_rate;
            _my_socket->send(reinterpret_cast<unsigned char*>(&prv), sizeof(Pitch_Rate_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting_
            Mass_Proxy::Value massv = *_mass_p;
            _my_socket->send(reinterpret_cast<unsigned char*>(&massv), sizeof(Mass_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting

            Odometer_Proxy::Value odometerv = *_odometer_p;
            _my_socket->send(reinterpret_cast<unsigned char*>(&odometerv), sizeof(Odometer_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting_

            IMU_ACC_X_Proxy::Value axv = *_ax;
            _my_socket->send(reinterpret_cast<unsigned char*>(&axv), sizeof(IMU_ACC_X_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting_

            IMU_ACC_Y_Proxy::Value ayv = *_ay;
            _my_socket->send(reinterpret_cast<unsigned char*>(&ayv), sizeof(IMU_ACC_Y_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting_

            IMU_ACC_Z_Proxy::Value azv = *_az;
            _my_socket->send(reinterpret_cast<unsigned char*>(&azv), sizeof(IMU_ACC_Z_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting_

            Throttle_Actuator_Proxy::Value tapv = *_tp;
            _my_socket->send(reinterpret_cast<unsigned char*>(&tapv), sizeof(Throttle_Actuator_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting_

            Brake_Actuator_Proxy::Value bapv = *_bp;
            _my_socket->send(reinterpret_cast<unsigned char*>(&bapv), sizeof(Brake_Actuator_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting_

            Steer_Actuator_Proxy::Value sapv = *_sp;
            _my_socket->send(reinterpret_cast<unsigned char*>(&sapv), sizeof(Steer_Actuator_Proxy::Value));
            _my_socket->receive(); // discard what i'm writting_

            db<SmartData>(INF) << "Motion_Planning::Awaiting response from Algorithm..." << endl;
            const unsigned char* data = _my_socket->receive();
            _value = *reinterpret_cast<const Value *>(data);
            for(unsigned int i = _my_socket->size(); i < sizeof(Value); i++) _value[i] = 0;
        #endif
        return true;
    }

private:
    Object_Recognition_And_Tracking_Fuser_Proxy *_fp;
    Path_Planning_Proxy *_pp;
    Dynamics_State_Proxy *_dp;
    Drag_Proxy *_drag_p;
    Gear_Proxy *_gear_p;
    Engine_RPM_Proxy *_erpm_p;
    Orientation_Proxy *_pitch_p;
    Mass_Proxy * _mass_p;
    Odometer_Proxy * _odometer_p;
    IMU_ACC_X_Proxy *_ax;
    IMU_ACC_Y_Proxy *_ay;
    IMU_ACC_Z_Proxy *_az;
    Pitch_Rate_Proxy *_pitch_rate;
    Throttle_Actuator_Proxy *_tp;
    Brake_Actuator_Proxy *_bp;
    Steer_Actuator_Proxy *_sp;
    Value _value;
    UDP_Socket * _my_socket;
    SmartData::Time _last_consumption;
};


using Motion_Planning = Responsive_SmartData<Motion_Planning_Transformer>;
