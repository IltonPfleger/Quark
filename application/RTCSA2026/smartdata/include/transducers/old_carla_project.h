#include <transducer.h>

class Camera_CARLA_Transducer : public Transducer<SmartData::Unit::CARLA_IMAGE|1> 
{
    friend Responsive_SmartData<Camera_CARLA_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;
    
public:
    Camera_CARLA_Transducer(const Device_Id & dev): _value(0) {
        // 5001 --> (0+1) Image socket (Carla -> SmartData)
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Camera_CARLA_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<const Value*>(_my_socket->data());
        for(UInt32 i = _my_socket->size(); i < (UNIT & SmartData::Unit::LEN); i++) _value[i] = 0;
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New image received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    UDP_Socket * _my_socket;
};

class Direction_Transducer: public Transducer<SmartData::Unit::Direction>
{
    friend Responsive_SmartData<Direction_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

public:
    Direction_Transducer(const Device_Id & dev): _value(0) {
        // 5002 --> (1+1) Direction socket (Carla -> SmartData)
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Direction_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = _my_socket->data()[0];
        notify(); // update() SmartData
    }

private:
    Value _value;
    UDP_Socket * _my_socket;
};

class Speed_Transducer: public Transducer<SmartData::Unit::Speed>
{
    friend Responsive_SmartData<Speed_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

public:
    Speed_Transducer(const Device_Id & dev): _value(0) {
        // 5003 --> (2+1) Image socket (Carla -> SmartData)
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Speed_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = std::stoi(reinterpret_cast<const char *>(_my_socket->data())); // 4B to int
        notify(); // update() SmartData
    }

private:
    Value _value;
    UDP_Socket * _my_socket;
};

class Acceleration_Remote_Transducer: public Transducer<SmartData::Unit::Acceleration>
{
    friend Responsive_SmartData<Acceleration_Remote_Transducer>;

public:
    static const bool active = false; // do not trigger interruptions
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR | ACTUATOR; //transformer result

public:
    Acceleration_Remote_Transducer(const Device_Id & dev): _value(0) {}

    ~Acceleration_Remote_Transducer() {}

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }

    void update(typename UDP_Socket::Observed *obs) {}

private:
    Value _value;
};

class Angle_Remote_Transducer: public Transducer<SmartData::Unit::Angle>
{
    friend Responsive_SmartData<Angle_Remote_Transducer>;

public:
    static const bool active = false; // do not trigger interruptions
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR | ACTUATOR; //transformer result

public:
    Angle_Remote_Transducer(const Device_Id & dev): _value(0) {}

    ~Angle_Remote_Transducer() {}

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }

    void update(typename UDP_Socket::Observed *obs) {}

private:
    Value _value;
};

class Angle_Local_Transducer: public Transducer<SmartData::Unit::Angle>
{
    friend Responsive_SmartData<Angle_Local_Transducer>;

public:
    static const bool active                   = false; // do not trigger interruptions
    static const Uncertainty UNCERTAINTY       = UNKNOWN;
    static const Type TYPE                     = ACTUATOR;
    static const UInt32 ACTUATION_SOCKET = 4;

public:
    Angle_Local_Transducer(const Device_Id & dev): _value(0), _dev(dev) {
        _my_socket = new UDP_Socket(dev == 3 ? ACTUATION_SOCKET : dev);
    }

    ~Angle_Local_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { 
        _value = value;
        char send_data[UDP_Socket::MTU];
        std::string aux = "";
        aux = "{\"value\": " + std::to_string(value) + ",\"dev\": "+ std::to_string(_dev) + "}";
        strcpy(send_data, aux.c_str());
        UInt32 size;
        for (size = 0; send_data[size] != '\0'; ++size);
        _my_socket->send(reinterpret_cast<unsigned char*>(send_data), size);
    }

    void update(typename UDP_Socket::Observed * obs) {}

private:
    Value _value;
    Device_Id _dev;
    UDP_Socket * _my_socket;
};

class Acceleration_Local_Transducer: public Transducer<SmartData::Unit::Acceleration>
{
    friend Responsive_SmartData<Acceleration_Local_Transducer>;
public:
    static const bool active = false; // do not trigger interruptions
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = ACTUATOR;
    static const UInt32 ACTUATION_SOCKET = 4;

public:
    Acceleration_Local_Transducer(const Device_Id & dev): _value(0), _dev(dev) {
        _my_socket = new UDP_Socket((dev == 4 || dev == 5) ? ACTUATION_SOCKET : dev);
    }

    ~Acceleration_Local_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) {
        _value = value;
        char send_data[UDP_Socket::MTU];
        std::string aux = "";
        aux = "{\"value\": " + std::to_string(value) + ",\"dev\": "+ std::to_string(_dev) + "}";
        strcpy(send_data, aux.c_str());
        UInt32 size;
        for (size = 0; send_data[size] != '\0'; ++size);
        _my_socket->send(reinterpret_cast<unsigned char*>(send_data), size);
    }

    void update(typename UDP_Socket::Observed * obs) {}

private:
    Value _value;
    Device_Id _dev;
    UDP_Socket * _my_socket;
};

class GPS3I_Transducer: public Transducer<SmartData::Unit::GPS3I>
{
    friend Responsive_SmartData<GPS3I_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

public:
    GPS3I_Transducer(const Device_Id & dev): _value(0) {
        // 5010 --> (9+1) Fake GPS socket (Carla -> SmartData)
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~GPS3I_Transducer() { delete _my_socket; delete _value; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<const Value*>(_my_socket->data());
        for(UInt32 i = _my_socket->size(); i < (UNIT & SmartData::Unit::LEN); i++) _value[i] = 0;
        notify(); // update() SmartData
    }

private:
    Value _value;
    UDP_Socket * _my_socket;
};

class Dynamics_Transducer: public Transducer<SmartData::Unit::Dynamics>
{
    friend Responsive_SmartData<Dynamics_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

public:
    Dynamics_Transducer(const Device_Id & dev): _value(0) {
        // 5011 --> (10+1) Fake LIDAR socket (Carla -> SmartData)
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Dynamics_Transducer() { delete _my_socket; delete _value; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<const Value*>(_my_socket->data());
        for(UInt32 i = _my_socket->size(); i < (UNIT & SmartData::Unit::LEN); i++) _value[i] = 0;
        notify();
    }

private:
    Value _value;
    UDP_Socket * _my_socket;
};

class Dynamics_Array_Transducer: public Transducer<SmartData::Unit::Dynamics_Array>
{
    friend Responsive_SmartData<Dynamics_Array_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

public:
    Dynamics_Array_Transducer(const Device_Id & dev): _value(0) {
        // 5011 --> (10+1) Fake LIDAR socket (Carla -> SmartData)
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Dynamics_Array_Transducer() { delete _my_socket; delete _value; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<const Value*>(_my_socket->data());
        for(UInt32 i = _my_socket->size(); i < (UNIT & SmartData::Unit::LEN); i++) _value[i] = 0;
        notify(); // update() SmartData
    }

private:
    Value _value;
    UDP_Socket * _my_socket;
};

using Camera_CARLA = Responsive_SmartData<Camera_CARLA_Transducer>;
using Camera_CARLA_Proxy = Interested_SmartData<SmartData::Unit::Wrap<Camera_CARLA_Transducer::UNIT>>;

using Direction = Responsive_SmartData<Direction_Transducer>;
using Direction_Proxy = Interested_SmartData<Direction_Transducer::Unit::Wrap<Direction_Transducer::UNIT>>;

using Angle_Remote = Responsive_SmartData<Angle_Remote_Transducer>;
using Angle_Remote_Proxy = Interested_SmartData<Angle_Remote_Transducer::Unit::Wrap<Angle_Remote_Transducer::UNIT>>;

using Acceleration_Remote = Responsive_SmartData<Acceleration_Remote_Transducer>;
using Acceleration_Remote_Proxy = Interested_SmartData<Acceleration_Remote_Transducer::Unit::Wrap<Acceleration_Remote_Transducer::UNIT>>;

using Speed = Responsive_SmartData<Speed_Transducer>;
using Speed_Proxy = Interested_SmartData<Speed_Transducer::Unit::Wrap<Speed_Transducer::UNIT>>;

using GPS3I = Responsive_SmartData<GPS3I_Transducer>;
using GPS3I_Proxy = Interested_SmartData<GPS3I_Transducer::Unit::Wrap<GPS3I_Transducer::UNIT>>;

using Dynamics = Responsive_SmartData<Dynamics_Transducer>;
using Dynamics_Proxy = Interested_SmartData<Dynamics_Transducer::Unit::Wrap<Dynamics_Transducer::UNIT>>;

using Dynamics_Array = Responsive_SmartData<Dynamics_Array_Transducer>;
using Dynamics_Array_Proxy = Interested_SmartData<Dynamics_Array_Transducer::Unit::Wrap<Dynamics_Array_Transducer::UNIT>>;

using Angle_Local = Responsive_SmartData<Angle_Local_Transducer>;
using Acceleration_Local = Responsive_SmartData<Acceleration_Local_Transducer>;
