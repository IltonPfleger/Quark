#include<transducer.h>

class Temperature_Transducer: public Transducer<SmartData::Unit::Temperature|SmartData::Unit::F32>, private UDP_Socket::Observer
{
    friend Responsive_SmartData<Temperature_Transducer>;
public:
    static const bool active = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR; // it should be transformer... :)

    typedef __UTIL::Observer Observer;

public:
    Temperature_Transducer(const Device_Id & dev): _value(0) {
        _my_socket = new UDP_Socket(dev+1); // Shared Memory with real transducer ... 
        // If real implementation is fixed, replace this with real hardware interaction
        // We are using shared memory to avoid recompiling the smartdata component
        // Therefore, replacing an implemention is simply changing who is writting in this socket
        _my_socket->attach(this);
    }

    ~Temperature_Transducer() { delete _my_socket; }

    virtual Value sense() { return _value; }

    virtual void actuate(const Value & value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename UDP_Socket::Observed *obs) {
        _value = *reinterpret_cast<Value *>(const_cast<unsigned char *>(_my_socket->data())); // 4B to Value
        notify(); // update() SmartData
        db<SmartData>(TRC) << "New Temperature sample received! t=" << _my_socket->reception() << endl;
    }

private:
    Value _value;
    UDP_Socket * _my_socket;
};

using Temperature = Responsive_SmartData<Temperature_Transducer>;