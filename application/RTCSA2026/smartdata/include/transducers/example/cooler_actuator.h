class Cooler_Actuator_Transducer: public Transducer<SmartData::Unit::Switch> // source is Temperature Adjust
{
    friend Responsive_SmartData<Cooler_Actuator_Transducer>;
public:
    static const bool active = false; // do not trigger interruptions
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = ACTUATOR;
public:
    Cooler_Actuator_Transducer(const Device_Id & dev): _value(0), _dev(dev) {
        _my_socket = new UDP_Socket(dev+1, false);
    }

    ~Cooler_Actuator_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) {
        db<SmartData>(TRC) << "New command received! v=" << value << endl;
        _value = value;
        _my_socket->send(reinterpret_cast<const unsigned char*>(&_value), 1); // on or off
    }

    void update(typename UDP_Socket::Observed * obs) {}
    virtual SmartData::Signature signature() { return 0; }

private:
    Value _value;
    Device_Id _dev;
    UDP_Socket * _my_socket;
};

using Cooler_Actuator = Responsive_SmartData<Cooler_Actuator_Transducer>; // no proxy since this is the last step of the actuation (actuate itself) -- interest into the actuation value must be issued to the source