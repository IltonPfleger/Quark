#include <transformer.h>

class Temperature_Adjust_Transformer: public Transducer<SmartData::Unit::Switch>, private Observer
{
    friend Responsive_SmartData<Temperature_Adjust_Transformer>;
    using Temperature_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Temperature|SmartData::Unit::F32)>>;

public:
    static const bool active = true;
    static const UInt64 EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;

    typedef __UTIL::Observer Observer;

public:
    Temperature_Adjust_Transformer(const Device_Id & dev) {
        _my_socket = new UDP_Socket(dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function

        // inputs
        _temp1 = new Temperature_Proxy(Temperature_Proxy::Region(0, 0, 0, 100, Temperature_Proxy::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 0);           // expected every 100 mili
        _ideal_temp = new Temperature_Proxy(Temperature_Proxy::Region(0, 0, 0, 100, Temperature_Proxy::now(),     INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 1);     // event driven, update is expected at least every expiry
        db<SmartData>(TRC) << "OBJRecTrack interests created!" << endl;
        // attach my inputs to trigger my update
        _temp1->attach(this);
        _ideal_temp->attach(this);
        _last_consumption = 0;
    }

    ~Temperature_Adjust_Transformer() { delete _my_socket; delete _temp1; delete _ideal_temp; }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        db<SmartData>(TRC) << "Temperature_Fuser_Transformer!" << (_temp1->when() > _last_consumption) << (_ideal_temp->when() > _last_consumption) << endl;
        if (!_temp1->expired() && _temp1->when() > _last_consumption && // wait for new data
             !_ideal_temp->expired()) { // ideal is updated on event driven basis
            _last_consumption = _temp1->now();
            // take care with size of when using Digital Data
            // in case of digital data use the information of value size: sizeof(Temperature_Proxy::Value)
            float t1 = *_temp1;
            float t2 = *_ideal_temp;
            unsigned char * ret = transform(reinterpret_cast<unsigned char*>(&t1), sizeof(Temperature_Proxy::Value), reinterpret_cast<unsigned char*>(&t2), sizeof(Temperature_Proxy::Value));
            _value = *reinterpret_cast<Value*>(ret);
            notify();
        }
    }

private:
    unsigned char * transform(unsigned char * _t1, UInt32 size_temp1, unsigned char * _t2, UInt32 size_temp2) {

        db<SmartData>(TRC) << "Inputs:" << size_temp1 << "," << size_temp2 << endl;

        _my_socket->send(_t1, size_temp1);
        _my_socket->receive(); // discard what i'm writting
        _my_socket->send(_t2, size_temp2);
        _my_socket->receive(); // discard what i'm writting

        db<SmartData>(TRC) << "Temperature_Fuser::Awaiting response from Algorithm..." << endl;
        return const_cast<unsigned char *>(_my_socket->receive());
    }

private:
    Temperature_Proxy *_temp1;
    Temperature_Proxy *_ideal_temp;

    Value _value;
    UDP_Socket * _my_socket;
    SmartData::Time _last_consumption;
};

using Temperature_Adjust = Responsive_SmartData<Temperature_Adjust_Transformer>;