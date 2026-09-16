#include <transducer.h>

class Map_Transducer : public Transducer<SmartData::Unit::MOTION_VECTOR_GLOBAL|Traits<Project>::MAX_MAP_POINTS>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<Map_Transducer>;

public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Map_Transducer(const Device_Id & dev): _value() {
        _my_socket = new UDP_Socket(dev+1);
        _my_socket->attach(this);
    }

    ~Map_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<const Value *>(_my_socket->data());
        for(UInt32 i = _my_socket->size(); i < sizeof(Value); i++) _value[i] = 0;
        _uncertainty = *reinterpret_cast<Uncertainty *>(const_cast<unsigned char *>(_my_socket->data())+sizeof(Value));

        notify(); // update() SmartData
        db<SmartData>(TRC) << "New Map received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    Uncertainty _uncertainty;
    UDP_Socket * _my_socket;
};

using Map = Responsive_SmartData<Map_Transducer>;
