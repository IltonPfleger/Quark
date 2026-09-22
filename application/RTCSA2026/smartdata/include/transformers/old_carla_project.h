#include <transformer.h>

class Imitation_Learning_Model_Tranformer: public Transducer<SmartData::Unit::Antigravity>, private Observer // Output is a multismartdata
{
    friend Responsive_SmartData<Imitation_Learning_Model_Tranformer>;

public:
    static const bool active = true;
    static const UInt64 EXPIRY = 10000000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR | ACTUATOR;

    typedef __UTIL::Observer Observer;

    struct control_values {
        int throttle;
        int break_intensity;
        int steer_angle;
    };

public:
    Imitation_Learning_Model_Tranformer(const Device_Id & dev){
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        _my_socket = new UDP_Socket(dev+1, false); // this socket is synchronous, it will only respond when we request --> controlled by transform function

        // inputs
        _cp  = new Camera_CARLA_Proxy(Camera_CARLA::      Region(0, 0, 0, 100, Camera_CARLA::now(),    INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 0);
        _dir = new Direction_Proxy(Direction::Region(0, 0, 0, 100, Direction::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 1);
        _sp  = new Speed_Proxy(Speed::        Region(0, 0, 0, 100, Speed::now(),     INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, 2);
        db<SmartData>(TRC) << "IML interest created!" << endl;
        // attach my inputs to trigger my update
        _cp->attach(this);
        _dir->attach(this);
        _sp->attach(this);

        // outputs
        _ang = new Angle_Remote(       3, EXPIRY, SmartData::ADVERTISED);
        _at  = new Acceleration_Remote(4, EXPIRY, SmartData::ADVERTISED);
        _br  = new Acceleration_Remote(5, EXPIRY, SmartData::ADVERTISED);

        // control of input update
        _last_consumption = 0;
    }

    ~Imitation_Learning_Model_Tranformer() { delete _my_socket; delete _cp; delete _dir; delete _sp; delete _at; delete _br; delete _ang; }

    virtual Value sense() { return 0; }

    virtual void actuate(const Value & value) {}

    void update(typename __UTIL::Observed *obs) {
        db<SmartData>(TRC) << "IML updated!" << _cp->expired() << _dir->expired() << _sp->expired() << endl;
        if (!_cp->expired() && _cp->when() > _last_consumption && !_dir->expired() && _dir->when() > _last_consumption && !_sp->expired() && _sp->when() > _last_consumption) {
            _last_consumption = _cp->now();
            struct control_values* cv = transform();
            *_ang = cv->steer_angle;
            *_at = cv->throttle;
            *_br = cv->break_intensity;
            delete cv;
        }
    }

private:
    struct control_values * transform() {

        Speed_Proxy::Value spv = *_sp;
        _my_socket->send(reinterpret_cast<const unsigned char *>(&spv), sizeof(Speed_Proxy::Value)); // sending in binary
        _my_socket->receive(); // discard what i'm writting
        Direction_Proxy::Value dirv = *_dir - '0';
        _my_socket->send(reinterpret_cast<const unsigned char *>(&dirv), sizeof(Direction_Proxy::Value)); // sending in binary
        _my_socket->receive(); // discard what i'm writting
        Camera_CARLA_Proxy::Value img = *_cp;
        _my_socket->send(img, sizeof(Camera_CARLA_Proxy::Value));
        _my_socket->receive(); // discard what i'm writting

        struct control_values * c = reinterpret_cast<struct control_values *>(malloc(sizeof(struct control_values)));

        db<SmartData>(INF) << "Awaiting response from AI..." << endl;
        const unsigned char *data = _my_socket->receive();
        db<SmartData>(INF) << "I've read changes{" << endl;
        std::string transformational_data = "";
        int i = 0;

        for(i = 0; data[i] != '\n'; i++) { transformational_data += data[i]; db<SmartData>(INF) << data[i];}
        c->throttle = std::stoi(transformational_data);
        transformational_data = "";
        db<SmartData>(INF) << "(t=" << c->throttle << ")" << endl;
        for(i = i+1; data[i] != '\n'; i++) { transformational_data += data[i]; db<SmartData>(INF) << data[i];}
        c->break_intensity = std::stoi(transformational_data);
        transformational_data = "";
        db<SmartData>(INF) << "(b=" << c->break_intensity << ")" << endl;

        for(i = i+1; data[i] != '\n'; i++) { transformational_data += data[i]; db<SmartData>(INF) << data[i];}
        c->steer_angle = std::stoi(transformational_data);
        db<SmartData>(INF) << "(s=" << c->steer_angle << ")" << endl;

        return c;
    }

private:
    Speed_Proxy *_sp;
    Camera_CARLA_Proxy *_cp;
    Direction_Proxy *_dir;

    Angle_Remote *_ang;
    Acceleration_Remote *_at;
    Acceleration_Remote *_br;

    UDP_Socket * _my_socket;
    SmartData::Time _last_consumption;
};

using Imitation_Learning_Model = Responsive_SmartData<Imitation_Learning_Model_Tranformer>;
