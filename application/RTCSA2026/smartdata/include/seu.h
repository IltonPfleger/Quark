#ifndef __SEU_SmartData__
#define __SEU_SmartData__

#include <network/tstp/tstp.h>
#include <smartdata.h>
#include <system/thread.h>
#ifdef CLOUD_INTEGRATION
#include <utility/iot/log_sender.h>
#endif
#include <utility/list.h>
#include <utility/observer.h>
#include <utility/stl.h>

SmartData::Time nanos() {
    SmartData::Time us = (unsigned long)QUARK::Nanosecond(QUARK::Timer::now());
    return us;
}

extern OStream kout, kerr;

class Unit_Dev_Expiry {
  public:
    typedef Simple_List<Unit_Dev_Expiry> List;

  public:
    Unit_Dev_Expiry(SmartData::Unit u = 0, UInt32 d = 0, SmartData::Time expiry = 0)
        : _unit(u),
          _dev(d),
          _expiry(expiry),
          _link(this) {}
    ~Unit_Dev_Expiry() { db<SmartData>(TRC) << "UD delete" << endl; }

    friend bool operator==(const Unit_Dev_Expiry &ud1, const Unit_Dev_Expiry &ud2) {
        db<SmartData>(TRC) << "Unit_Dev_Expiry::operator!=(this=" << ud1._unit << "," << ud1._dev << "," << ud1._expiry
                           << ";other=" << ud2._unit << "," << ud2._dev << "," << ud2._expiry << ")" << endl;
        return (ud1._unit == ud2._unit && ud1._dev == ud2._dev && ud1._expiry == ud2._expiry);
    }

    friend bool operator!=(const Unit_Dev_Expiry &ud1, const Unit_Dev_Expiry &ud2) { return !(operator==(ud1, ud2)); }

    friend bool operator<=(const Unit_Dev_Expiry &ud1, const Unit_Dev_Expiry &ud2) {
        bool ret = ud1._dev == ud2._dev;
        if (ret) return ud1._expiry <= ud2._expiry;
        return ret;
    }

    friend bool operator>=(const Unit_Dev_Expiry &ud1, const Unit_Dev_Expiry &ud2) {
        bool ret = ud1._dev == ud2._dev;
        if (ret) return ud1._expiry >= ud2._expiry;
        return ret;
    }

    friend Unit_Dev_Expiry operator-(const Unit_Dev_Expiry &ud1, const Unit_Dev_Expiry &ud2) {
        return ud1; // just for compiling, not used!
    }

    friend Unit_Dev_Expiry operator+(const Unit_Dev_Expiry &ud1, const Unit_Dev_Expiry &ud2) {
        return ud1; // just for compiling, not used!
    }

    friend OStream &operator<<(OStream &db, const Unit_Dev_Expiry &ude) {
        db << "(u=" << ude._unit << ",d=" << ude._dev << ",e=" << ude._expiry << ")";
        return db;
    }

    List::Element *link() { return &_link; }

    SmartData::Unit unit() { return _unit; }
    UInt32 dev() { return _dev; }
    SmartData::Time expiry() { return _expiry; }

  private:
    SmartData::Unit _unit;
    UInt32 _dev;
    SmartData::Time _expiry;
    List::Element _link;
};

class Boolean_Filter;

class Verifiable_SmartData : public SmartData, public TSTP::Observer {
  public:
    typedef Simple_Ordered_List<Verifiable_SmartData, Unit_Dev_Expiry> List;
    typedef unsigned char Data;
    typedef TSTP Network;
    typedef TSTP::Buffer Buffer;
    typedef TSTP::Timekeeper Timekeeper;
    typedef Boolean_Filter Observer;
    typedef Simple_List<Observer>::Iterator Iterator;

  public:
    static inline const UInt32 ITERATIONS_MEASURE = 1000;

  public:
    Verifiable_SmartData(UInt32 u, UInt32 d, Time e, Region r)
        : _ude(Unit_Dev_Expiry(u, d, e)),
          _region(r),
          _in_use(0),
          _link(this, _ude) {
        db<SmartData>(TRC) << "Verifiable_SmartData[SEU](u=" << u << ",d=" << d << ",e=" << e << ",r=" << r << ")" << endl;
        Network::attach(this);
        _value = new Data[_ude.unit().value_size()];
        if (_ude.unit().value_size() > 8) *reinterpret_cast<UInt32 *>(_value) = 0x0;
        _sum   = 0;
        _count = 0;
    }

    ~Verifiable_SmartData() {
        Network::detach(this);
        delete _value;
    }

    List::Element *link() { return &_link; }
    List::Element *new_link() { return new List::Element(this, _ude); }
    Time expiry() { return _ude.expiry(); }
    UInt32 dev() { return _ude.dev(); }
    Unit unit() { return _ude.unit(); }
    static Time now() { return Timekeeper::now(); }
    bool expired() { return Timekeeper::now() > (_response.origin().time + _ude.expiry()); }
    template <typename T> T *value() { return reinterpret_cast<T *>(_value); }
    void use() { _in_use++; }
    void remove() { --_in_use; }
    bool in_use() { return _in_use > 0; }
    Time origin_time() { return _response.origin().time; }
    Unit_Dev_Expiry unit_dev() { return _ude; }
#ifdef CLOUD_INTEGRATION
    void create_iot_series() {
        struct IoTLogs::IoTSeries series = {
            .unit      = _ude.unit(),
            .dev       = _ude.dev(),
            .signature = Traits<Project>::EGO_ID,
            .t0        = (UInt64)now(),
            .t1        = (UInt64)(now() + 1e10),
            .r         = 0,
            .x         = 0,
            .y         = 0,
            .z         = 0,
        };
        sender.create_series(series);
    }

    void add_sd_to_iot_queue() {
        std::string value = "";
        if (((UInt32)(_ude.unit()) & SmartData::Unit::SID) == SmartData::Unit::SI) {
            switch (((UInt32)(_ude.unit()) & SmartData::Unit::NUM)) {
                case SmartData::Unit::I32: value = std::to_string(*reinterpret_cast<Int32 *>(_value)); break;
                case SmartData::Unit::I64: value = std::to_string(*reinterpret_cast<Int64 *>(_value)); break;
                case SmartData::Unit::F32: value = std::to_string(*reinterpret_cast<float *>(_value)); break;
                default: value = std::to_string(*reinterpret_cast<double *>(_value)); break;
            }
        }
        struct IoTLogs::IoTSmartData sd = {
            //.value      = value.c_str(),
            .unit       = _ude.unit(),
            .dev        = _ude.dev(),
            .error      = 0,
            .confidence = 1,
            .signature  = Traits<Project>::EGO_ID,
            .workflow   = 0,
            .r          = 0,
            .t          = (UInt64)origin_time(),
            .x          = 0,
            .y          = 0,
            .z          = 0,
        };
        strcpy(sd.value, value.c_str());
        if (value.length() > 0) sender.add_to_queue(sd);
    }

    void set_mv(Motion_Vector m) {
        // WGS84
        static constexpr double a         = 6378137.0;
        static constexpr double e_squared = 0.00669437999014;
        double longitude                  = (double)m._location[0] / (double)Traits<Project>::FLOAT_INT_PRECISION;
        double latitude                   = (double)m._location[1] / (double)Traits<Project>::FLOAT_INT_PRECISION;
        double altitude                   = (double)m._location[2] / (double)Traits<Project>::FLOAT_INT_PRECISION_M;
        double latitude_sin               = sin(latitude);
        double longitude_sin              = sin(longitude);
        double latitude_cos               = cos(latitude);
        double longitude_cos              = cos(longitude);
        double N                          = a / (sqrt(1 - e_squared * latitude_sin * latitude_sin));
        double x                          = (N + altitude) * latitude_cos * longitude_cos;
        double y                          = (N + altitude) * latitude_cos * longitude_sin;
        double z                          = ((1 - e_squared) * N + altitude) * latitude_sin;
        sender.new_location(x, y, z);
    }
#endif
    void log_state() { // TODO: make this a "<<" operator
        if (Traits<Build>::cloud) {
#ifdef CLOUD_INTEGRATION
            add_sd_to_iot_queue();
#endif
        } else {
            if (((UInt32)(_ude.unit()) & SmartData::Unit::SID) == SmartData::Unit::SI) {
                db<SmartData>(LOGGER) << "(u=" << _ude.unit() << "=>" << (UInt32)_ude.unit() << ",d=" << _ude.dev()
                                      << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={";
                switch (((UInt32)(_ude.unit()) & SmartData::Unit::NUM)) {
                    case SmartData::Unit::I32: db<SmartData>(LOGGER) << *reinterpret_cast<Int32 *>(_value); break;
                    case SmartData::Unit::I64: db<SmartData>(LOGGER) << *reinterpret_cast<Int64 *>(_value); break;
                    case SmartData::Unit::F32: db<SmartData>(LOGGER) << *reinterpret_cast<float *>(_value); break;
                    default: db<SmartData>(LOGGER) << *reinterpret_cast<double *>(_value); break;
                }
            } else if (_ude.unit() == SmartData::Unit(SmartData::Unit::MOTION_VECTOR_LOCAL) ||
                       _ude.unit() == SmartData::Unit(SmartData::Unit::MOTION_VECTOR_GLOBAL)) {
                db<SmartData>(LOGGER) << "(u=" << _ude.unit() << "=>" << (UInt32)_ude.unit() << ",d=" << _ude.dev()
                                      << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={\n\t[";
                log_value_to_mvs();
                db<SmartData>(LOGGER) << "\n\t]";
            } else if (_ude.unit() == SmartData::Unit(SmartData::Unit::WHEEL_TELEMETRY)) {
                Wheel_Telemetry_Value *wt = reinterpret_cast<Wheel_Telemetry_Value *>(_value);
                db<SmartData>(LOGGER) << "(u=" << SmartData::Unit(SmartData::Unit::Speed | SmartData::Unit::F32) << "=>"
                                      << (UInt32)SmartData::Unit(SmartData::Unit::Speed | SmartData::Unit::F32) << ",d=" << _ude.dev()
                                      << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={" << wt->_wheel_speed << "}\n";
                db<SmartData>(LOGGER) << "(u=" << SmartData::Unit(SmartData::Unit::Force | SmartData::Unit::F32) << "=>"
                                      << (UInt32)SmartData::Unit(SmartData::Unit::Force | SmartData::Unit::F32) << ",d=" << _ude.dev() + 4
                                      << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={" << wt->_wheel_lat_force
                                      << "}\n";
                db<SmartData>(LOGGER) << "(u=" << SmartData::Unit(SmartData::Unit::Force | SmartData::Unit::F32) << "=>"
                                      << (UInt32)SmartData::Unit(SmartData::Unit::Force | SmartData::Unit::F32)
                                      << ",d=" << _ude.dev() + 4 * 2 << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={"
                                      << wt->_wheel_lat_force << "}\n";
                db<SmartData>(LOGGER) << "(u=" << SmartData::Unit(SmartData::Unit::Mass | SmartData::Unit::F32) << "=>"
                                      << (UInt32)SmartData::Unit(SmartData::Unit::Mass | SmartData::Unit::F32)
                                      << ",d=" << _ude.dev() + 4 * 3 << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={"
                                      << wt->_wheel_torque << "}\n";
                db<SmartData>(LOGGER) << "(u=" << SmartData::Unit(SmartData::Unit::Torque | SmartData::Unit::F32) << "=>"
                                      << (UInt32)SmartData::Unit(SmartData::Unit::Torque | SmartData::Unit::F32)
                                      << ",d=" << _ude.dev() + 4 * 4 << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={"
                                      << wt->_wheel_torque << "}\n";
                db<SmartData>(LOGGER) << "(u=" << SmartData::Unit(SmartData::Unit::Angle | SmartData::Unit::F32) << "=>"
                                      << (UInt32)SmartData::Unit(SmartData::Unit::Angle | SmartData::Unit::F32)
                                      << ",d=" << _ude.dev() + 4 * 5 << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={"
                                      << wt->_wheel_lat_slip << "}\n";
                db<SmartData>(LOGGER) << "(u=" << SmartData::Unit(SmartData::Unit::Angle | SmartData::Unit::F32) << "=>"
                                      << (UInt32)SmartData::Unit(SmartData::Unit::Angle | SmartData::Unit::F32)
                                      << ",d=" << _ude.dev() + 4 * 6 << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={"
                                      << wt->_wheel_long_slip << "}\n";
                db<SmartData>(LOGGER) << "(u=" << SmartData::Unit(SmartData::Unit::Ratio | SmartData::Unit::F32) << "=>"
                                      << (UInt32)SmartData::Unit(SmartData::Unit::Ratio | SmartData::Unit::F32)
                                      << ",d=" << _ude.dev() + 4 * 7 << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={"
                                      << wt->_wheel_tire_friction;
            } else {
                db<SmartData>(LOGGER) << "(u=" << _ude.unit() << "=>" << (UInt32)_ude.unit() << ",d=" << _ude.dev()
                                      << ",t=" << origin_time() << ",sig=" << _response.signature() << ")={";
                switch (_ude.unit() >> 16) {
                    case SmartData::Unit::PCD_MONOCROMATIC >> 16: // for this implementaiton unit is the same as CLOUD_POINTS_RADAR
                    case SmartData::Unit::RAW_BGR >> 16:          // camera
                        db<SmartData>(LOGGER) << "size=" << _ude.unit().value_size();
                        // if (*reinterpret_cast<UInt32 *>(_value) < 0xFFFFFFFF)
                        //     for (unsigned i = 0; i < _ude.unit().value_size(); i++)
                        //         db<SmartData>(LOGGER) << hex << _value[i];
                        break;
                    case SmartData::Unit::Switch >> 16: // hand_brake, reverse
                        db<SmartData>(LOGGER) << (*reinterpret_cast<bool *>(_value) ? "True" : "False");
                        break;
                    default: db<SmartData>(LOGGER) << "Unknown how to parse unit: " << (UInt32)_ude.unit() << endl; break;
                }
            }
            db<SmartData>(LOGGER) << "}" << endl;
        }
    }

  public:
    void attach(Observer *o);
    void detach(Observer *o);

  private:
    bool notify(Verifiable_SmartData *d);

  private:
    void update(typename Network::Observed *obs, Buffer *buffer) {
        db<SmartData>(TRC) << "SmartData[SEU]::update(obs=" << obs << ",buf=" << buffer << ")" << endl;
        Header *header = buffer->frame()->template data<Header>();
        switch (header->type()) {
            case RESPONSE: {
                Response *response = buffer->frame()->template data<Response>();
                db<SmartData>(INF) << "SmartData[SEU]::update:msg=" << response->origin() << "d=" << response->device() << ", I=" << _region
                                   << ",d=" << _ude.dev() << endl;

                db<SmartData>(LOGGER) << response->unit() << " " << _ude.unit() << endl;
                db<SmartData>(LOGGER) << response->device() << " " << _ude.dev() << endl;
                db<SmartData>(LOGGER) << response->origin() << " " << _region << endl;

                if ((response->unit() == _ude.unit()) && _region.contains(response->origin()) && response->device() == _ude.dev()) {
                    if ((response->operation()) != ADVERTISE) {
                        Time start = nanos();
                        db<SmartData>(INF) << "SmartData[SEU]::update:msg=" << *response << endl;
                        _response = *response;
                        if ((_ude.unit() >> 16) != (SmartData::Unit::RAW_BGR >> 16) &&
                            (_ude.unit() >> 16) != (SmartData::Unit::PCD_MONOCROMATIC >> 16))
                            memcpy(reinterpret_cast<unsigned char *>(_value),
                                   reinterpret_cast<const unsigned char *>(&(response->template value<Data>())), _ude.unit().value_size());
                        db<SmartData>(INF) << "SmartData[SEU]::Calling notify()" << endl;
                        if (!Traits<Build>::verified) log_state();
                        notify(this); // notify boolean filters
                        db<SmartData>(INF) << "SmartData[SEU]::notified()" << endl;
                        _sum = _sum + (nanos() - start);
                        if (_count == ITERATIONS_MEASURE) {
                            db<SmartData>(LOGGER) << _ude.dev() << " ==> SUM of " << ITERATIONS_MEASURE << "=" << ((double)_sum) / 1000.
                                                  << ",avg=" << (((double)_sum) / 1000.) / ITERATIONS_MEASURE << endl;
                        }
                        _count++;
                    }
                } else
                    db<SmartData>(INF) << "SmartData[SEU]::update: not interested!" << endl;
            } break;
            default: {
            } break;
        }
    }

    void log_value_to_mvs() {
        UInt32 mv_size = _ude.unit().value_size() / (_ude.unit() & SmartData::Unit::LEN); // total len / amount of data inside
        for (UInt32 index = 0; index < _ude.unit().value_size(); index += mv_size) {
            if (!*reinterpret_cast<bool *>(&_value[index])) // if empty
                break;

            db<SmartData>(LOGGER) << "\n\t{";
            // reinterpret_cast<Motion_Vector *>(&_value[index])->log();
            log_value_to_mv(index + 1);
            db<SmartData>(LOGGER) << "\n\t";
            if (index + mv_size < _ude.unit().value_size())
                db<SmartData>(LOGGER) << "},";
            else
                db<SmartData>(LOGGER) << "}";
        }
    }

  protected:
    void log_value_to_mv(UInt32 index = 0, UInt32 size = 4) {
        // Speed XYZ (12B), Acceleration XYZ (12B), YPR Rate (12B), Location XYZ (12B), YPR (12B), Steer(4B), Battery
        // State of Charge (4B), Engine RPM (4B), Gear (4B)
        db<SmartData>(LOGGER) << "lon=" << *reinterpret_cast<int *>(&_value[index]) << ",\n\t";
        index += size;
        db<SmartData>(LOGGER) << "lat=" << *reinterpret_cast<int *>(&_value[index]) << ",\n\t";
        index += size;
        db<SmartData>(LOGGER) << "alt=" << *reinterpret_cast<int *>(&_value[index]) << ",\n\t";
        index += size;
        db<SmartData>(LOGGER) << "speed=" << *reinterpret_cast<float *>(&_value[index]) << ",\n\t";
        index += size;
        db<SmartData>(LOGGER) << "heading=" << *reinterpret_cast<float *>(&_value[index]) << ",\n\t";
        index += size;
        db<SmartData>(LOGGER) << "yawr=" << *reinterpret_cast<float *>(&_value[index]) << ",\n\t";
        index += size;
        db<SmartData>(LOGGER) << "accel=" << *reinterpret_cast<float *>(&_value[index]) << ",\n\t";
        index += size;
        db<SmartData>(LOGGER) << "id=" << *reinterpret_cast<UInt64 *>(&_value[index]) << ",\n\t";
        index += size * 2;
        db<SmartData>(LOGGER) << "uncertainty=" << *reinterpret_cast<float *>(&_value[index]) << ",\n\t";
        index += size;
        db<SmartData>(LOGGER) << "class=" << *reinterpret_cast<int *>(&_value[index]) << "\n\t";
    }

  protected:
#ifdef CLOUD_INTEGRATION
    IoTLogs::IoTSender sender;
#endif
    Unit_Dev_Expiry _ude;
    Region _region;
    UInt32 _in_use;
    List::Element _link;
    Data *_value;
    Simple_List<Boolean_Filter> _observers;
    Time _sum;
    UInt32 _count;
    Response _response;
};

class SEU_SmartData;
class STL_Verifier_Interest;

class Boolean_Filter {
    friend class STL_Verifier_Interest;
    friend class SEU_SmartData;

  public:
    typedef Simple_List<Boolean_Filter> List;
    typedef STL_Verifier_Interest Observer;
    typedef Simple_List<Observer>::Iterator Iterator;

  public:
    Boolean_Filter(Unit_Dev_Expiry::List *supported, Unit_Dev_Expiry ude, Microsecond period = 0)
        : _truth_value(true),
          _time(0),
          _supported_SmartData(supported),
          _ude(ude),
          _period(period),
          _link(this),
          _link2(this) {
        _inputs    = new Verifiable_SmartData::List();
        _SmartData = 0;

        for (Unit_Dev_Expiry::List::Iterator it = _supported_SmartData->begin(); it != _supported_SmartData->end(); it++) {
            db<SmartData>(TRC) << "Boolean_Filter::supported_input(ud=" << it->object()->unit() << ",d=" << it->object()->dev() << ")"
                               << endl;
        }
    }

    virtual ~Boolean_Filter() {
        db<SmartData>(TRC) << "BF delete" << endl;
        for (Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++)
            it->object()->detach(this);
        while (!_inputs->empty()) {
            Verifiable_SmartData::List::Element *sd = _inputs->remove_head();
            delete sd->object();
            delete sd;
        }
        delete _inputs;
        while (!_supported_SmartData->empty())
            delete _supported_SmartData->remove_head()->object();
        delete _supported_SmartData;
    }

    bool result() { return _truth_value; }
    Int64 when() { return _time; }
    Unit_Dev_Expiry unit_dev() { return _ude; }
    Microsecond period() { return _period; }
    List::Element *link() { return &_link; }
    List::Element *link2() { return &_link2; }
    List::Element *new_link() { return new List::Element(this); }

    virtual bool register_smartdata(Verifiable_SmartData *sd) {
        if (sd->unit_dev() == unit_dev() && _SmartData == 0) {
            _SmartData = sd;
            sd->attach(this);
            db<SmartData>(TRC) << "Boolean_Filter::register_smartdata: registered " << sd->unit_dev() << " in bf for " << unit_dev()
                               << ",equal?" << (sd->unit_dev() == unit_dev()) << ",_SmartData=" << _SmartData << endl;
            return true;
        }
        db<SmartData>(TRC) << "Boolean_Filter::register_smartdata: attemped to register " << sd->unit_dev() << " in bf for " << unit_dev()
                           << ",equal?" << (sd->unit_dev() == unit_dev()) << ",_SmartData=" << _SmartData << endl;
        return false;
    }

    virtual bool add_input(Verifiable_SmartData *sd) {
        Unit_Dev_Expiry ud = sd->unit_dev();
        db<SmartData>(LOGGER) << "Boolean_Filter()::add_input(ud=" << sd->unit() << ",d=" << sd->dev() << ",e=" << sd->expiry()
                           << "),this=" << unit_dev() << endl;
        Unit_Dev_Expiry *ude = 0;
        for (Unit_Dev_Expiry::List::Iterator it = _supported_SmartData->begin(); it != _supported_SmartData->end(); it++) {
            if (*it->object() == ud) {
                ude = it->object();
                break;
            }
        }

        if (ude != 0) {
            db<SmartData>(LOGGER) << "Boolean_Filter::add_input: interested! Attached SD=" << sd << endl;
            sd->attach(this);
            _inputs->insert(sd->new_link());
            sd->use(); // for registering only
            _supported_SmartData->remove(ude->link());
            db<SmartData>(INF) << "Boolean_Filter::add_input: _supported_SmartData->size()=" << _supported_SmartData->size() << endl;
            db<SmartData>(INF) << "Boolean_Filter::add_input: _inputs->size()=" << _inputs->size() << endl;
            db<SmartData>(INF) << "Boolean_Filter::add_input: _inputs->head()->next()=" << _inputs->head()->next() << endl;
            delete ude; // need to fix on main
            return true;
        } else {
            db<SmartData>(INF) << "Boolean_Filter::add_input: not_interested!" << endl;
            return false;
        }
    }

    virtual void remove_input(Verifiable_SmartData *sd) {
        Verifiable_SmartData::List::Element *sde = _inputs->remove_rank(sd->unit_dev());
        if (sde) {
            _supported_SmartData->insert((new Unit_Dev_Expiry(sd->unit_dev()))->link());
            sd->detach(this);
            sd->remove();
            if (!sd->in_use()) delete sd;
            delete sde;
        }
    }

    virtual void remove_input(Unit_Dev_Expiry ude) {
        Verifiable_SmartData::List::Element *sde = _inputs->remove_rank(ude);
        if (sde) {
            _supported_SmartData->insert((new Unit_Dev_Expiry(ude))->link());
            sde->object()->detach(this);
            sde->object()->remove();
            if (!sde->object()->in_use()) delete sde->object();
            delete sde;
        }
    }

    friend OStream &operator<<(OStream &db, const Boolean_Filter &d) {
        d.description();
        db << " - Safe=" << (d._truth_value ? "True" : "False");
        return db;
    }

  public:
    virtual void update(Verifiable_SmartData *vsd) {
        // Verifiable_SmartData * vsd = reinterpret_cast<Verifiable_SmartData *>(obs);
        db<SmartData>(TRC) << "Boolean_Filter[SEU]::notified():bf=" << this << "=>" << unit_dev() << ",by=" << vsd->unit_dev()
                           << ",addr=" << vsd << endl;
        SmartData::Time t = TSC::time_stamp();
        if (evaluate()) { // if not enough data is found to evaluate, return false
            notify(this);
            _elapsed = TSC::time_stamp() - t;
        } else {
            db<SmartData>(INF) << "\tNot enough data found to evaluate, will not notify!" << endl;
        }
    }

    void attach(Observer *o);
    void detach(Observer *o);

  private:
    bool notify(Boolean_Filter *d);

    virtual bool evaluate()          = 0;
    virtual void description() const = 0;

  protected:
    bool _truth_value;
    Int64 _time;
    Unit_Dev_Expiry::List *_supported_SmartData;
    Unit_Dev_Expiry _ude;
    Microsecond _period;
    List::Element _link;
    List::Element _link2;
    Verifiable_SmartData::List *_inputs;
    Verifiable_SmartData *_SmartData;
    Simple_List<Observer> _observers;
    SmartData::Time _elapsed;
};

class MU_Arrival : public Boolean_Filter,
                   SmartData // basic boolean filter
{
  public:
    MU_Arrival(SmartData::Unit unit, UInt32 dev, Time expiry, Microsecond period = 0)
        : Boolean_Filter(new Unit_Dev_Expiry::List(), Unit_Dev_Expiry(unit, dev, expiry), period) {
        db<SmartData>(TRC) << "SmartData[SEU]::MU_Arrival::this=" << this << endl;
    }
    ~MU_Arrival() {}

  private:
    virtual bool evaluate() {
        db<SmartData>(ERR) << "MU_Arrival[SEU]::evaluate()::this=" << this << endl;
        if (_SmartData != 0) {
            _truth_value = true; // only called when bf update is called, which is only triggered when data is updated
            _time        = Microsecond(_SmartData->origin_time());
        } else {
            _truth_value = false;
            _time        = TSC::time_stamp();
        }
        return true;
    }

    virtual void description() const {
        // if (_SmartData != 0)
        //     db << "MU_Arrival((u=" << _ude.unit() << ",d=" << _ude.dev() << ",expired?=" << _SmartData->expired()
        //     <<")={";
        // else
        //     db << "MU_Arrival((u=" << unit_dev().unit() << ",d=" << unit_dev().dev() << ",expired?=1)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")= (t=" <<
        //     it->object()->origin_time() << ", expired?=" << it->object()->expired() <<")";
        // }
        // db << "}";
    }
};

class MU_Arrival_Dep : public Boolean_Filter,
                       SmartData // basic boolean filter
{
  public:
    MU_Arrival_Dep(Unit_Dev_Expiry::List *supported_SmartData, SmartData::Unit unit, UInt32 dev, Time expiry, Microsecond period = 0)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(unit, dev, expiry), period),
          _last_update(0) {
        db<SmartData>(TRC) << "SmartData[SEU]::MU_Arrival_Dep::this=" << this << endl;
    }
    ~MU_Arrival_Dep() {}

  private:
    virtual bool evaluate() {
        db<SmartData>(TRC) << "MU_Arrival_Dep[SEU]::evaluate()::this=" << unit_dev() << endl;
        if (_SmartData != 0 && _SmartData->origin_time() > _last_update) {
            _truth_value = true;
            _time        = Microsecond(_SmartData->origin_time());
            _last_update = _SmartData->origin_time();
            return true;
        }
        return false;
    }

    virtual void description() const {
        // if (_SmartData != 0)
        //     db << "MU_Arrival((u=" << unit_dev().unit() << ",d=" << unit_dev().dev() << ",expired?=" <<
        //     _SmartData->expired() <<")={";
        // else
        //     db << "MU_Arrival((u=" << unit_dev().unit() << ",d=" << unit_dev().dev() << ",expired?=1)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")= (t=" <<
        //     it->object()->origin_time() << ", expired?=" << it->object()->expired() <<")";
        // }
        // db << "}";
    }

  private:
    SmartData::Time _last_update;
};

class MU_Expired : public Boolean_Filter,
                   SmartData // basic boolean filter
{
  public:
    MU_Expired(Unit_Dev_Expiry::List *supported_SmartData, SmartData::Unit unit, UInt32 dev, Time expiry, Microsecond period = 0)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(unit, dev, expiry), period) {
        db<SmartData>(TRC) << "SmartData[SEU]::Environment_Min_Max_Temperature_Boolean_Filter::this=" << this << endl;
    }
    ~MU_Expired() {}

  private:
    virtual bool evaluate() {
        db<SmartData>(TRC) << "MU_Arrival[SEU]::evaluate()::this=" << this << endl;
        if (_SmartData != 0) {
            _truth_value = _SmartData->expired();
            _time        = Microsecond(_SmartData->origin_time());
        } else {
            _truth_value = false;
            _time        = TSC::time_stamp();
        }
        return true;
    }

    virtual void description() const {
        // if (_SmartData != 0)
        //     db << "MU_Arrival((u=" << unit_dev().unit() << ",d=" << unit_dev().dev() << ",expired?=" <<
        //     _SmartData->expired() <<")={";
        // else
        //     db << "MU_Arrival((u=" << unit_dev().unit() << ",d=" << unit_dev().dev() << ",expired?=1)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")= (t=" <<
        //     it->object()->origin_time() << ", expired?=" << it->object()->expired() <<")";
        // }
        // db << "}";
    }
};

class STL_Verifier_Interest : public Observed {
    friend class SEU_SmartData;
    friend class Boolean_Filter;

  public:
    typedef Simple_List<STL_Verifier_Interest> List;
    typedef __UTIL::Observer Observer;

  public:
    STL_Verifier_Interest(Microsecond expiry, Microsecond period, Boolean_Filter *interested)
        : _period(period),
          _extended_stl_rule(0),
          _verifier_thread(0),
          _interested(interested),
          _interest(0),
          _link(this) {
        if (period <= expiry) {
            _extended_stl_rule = new Before(expiry);
            _period            = expiry;
        } else {
            _extended_stl_rule = new Eventually_Before(expiry, period);
        }
        _interested->attach(this);
        _interest = 0;
        _sum      = 0;
        _count    = 0;
        _sizes    = 0;
        if (expiry > period && period > 0)
            _verifier_thread = new Periodic_Thread(QUARK::Microsecond(expiry), &evaluate, reinterpret_cast<void *>(this));
        else if (period > 0)
            _verifier_thread = new Periodic_Thread(QUARK::Microsecond(period), &evaluate, reinterpret_cast<void *>(this));
    }

    virtual ~STL_Verifier_Interest() {
        if (_verifier_thread != 0) delete _verifier_thread;
        delete _extended_stl_rule;
        _interested->detach(this);
        if (_interest != 0) _interest->detach(this);
    }

    void update(Boolean_Filter *bf) {
        db<SmartData>(TRC) << "STL_Verifier_Interest[SEU]::notified()(bf=" << bf << "=" << bf->unit_dev() << ",interested=" << _interested
                           << "=" << _interested->unit_dev() << ",interest=" << _interest->unit_dev() << ",this=" << this << ")" << endl;
        _start        = true;
        bool s        = bf->result();
        Microsecond t = bf->when();
        db<SmartData>(INF) << "STL_Verifier_Interest[SEU]::(bf->result()=" << s << ")" << endl;
        if (bf == _interested) {
            db<SmartData>(INF) << s << "," << t << endl;
            _extended_stl_rule->add_sample(t, s, false);
            if (_period == 0) {
                _extended_stl_rule->evaluate(t);
                db<SmartData>(INF) << "STL_Verifier_Interest[SEU]::Event_Driven::result=" << result() << ",t=" << t << "}" << endl;
                notify(); // if period == 0, event driven, otherwise periodic and verified by thread
            }
        } else {
            _extended_stl_rule->add_sample(t, s, true);
        }
    }

    Unit_Dev_Expiry interest() { return _interest == 0 ? Unit_Dev_Expiry() : _interest->unit_dev(); }
    Boolean_Filter *interested() { return _interested; }
    bool result() const { return _extended_stl_rule->out(); }

    void register_boolean_filter_data_source(Boolean_Filter *bf) {
        bf->attach(this);
        _interest = bf;
    }

    List::Element *link() { return &_link; }
    List::Element *new_link() { return new List::Element(this); }

  private:
    static void *evaluate(void *p);

  protected:
    Microsecond _period;
    volatile bool _start;
    Extended_STL *_extended_stl_rule;
    Periodic_Thread *_verifier_thread;
    Boolean_Filter *_interested;
    Boolean_Filter *_interest;
    SmartData::Time _sum;
    UInt32 _count;
    int _sizes;
    List::Element _link;
};

// SmartData encapsulating remote transducers
class SEU_SmartData : public SmartData, private TSTP::Observer, private STL_Verifier_Interest::Observer {
  public:
    typedef TSTP Network;
    typedef Network::Buffer Buffer;
    typedef Network::Timekeeper Timekeeper;

  public:
    SEU_SmartData()
        : _safe(true) {
        db<SmartData>(LOGGER) << "SmartData[SEU]::this=" << reinterpret_cast<void *>(this) << endl;
        Network::attach(this); // Rank ALL for Data Observers
        _boolean_filters = new Boolean_Filter::List();
        _stl_verifiers   = new STL_Verifier_Interest::List();
        if (Traits<Build>::cloud) {
            _devices = new Verifiable_SmartData::List();
        }
        _start  = false;
        _period = 0;
        _thread = new Periodic_Thread(100000, &hyperperiod_verification, reinterpret_cast<void *>(this));
    }

    ~SEU_SmartData() {
        db<SmartData>(TRC) << "~SmartData[SEU](this=" << reinterpret_cast<void *>(this) << ")" << endl;
        Network::detach(this); // Rank ALL for Data Observers
        while (!_boolean_filters->empty())
            delete _boolean_filters->remove_head()->object();
        delete _boolean_filters;
        while (!_stl_verifiers->empty())
            delete _stl_verifiers->remove_head()->object();
        delete _stl_verifiers;
        if (_thread) delete _thread;
    }

    static Time now() { return Timekeeper::now(); }

    static void *hyperperiod_verification(void *p) {
        SEU_SmartData *d = reinterpret_cast<SEU_SmartData *>(p);
        while (!d->_start) {
            Thread::yield();
        }
        // usleep(d->_period);
        while (true) {
            Periodic_Thread::wait();
            //    for (STL_Verifier_Interest::List::Iterator it = d->_stl_verifiers->begin(); it !=
            //    d->_stl_verifiers->end();
            //         it++) {
            //        if (it->object()->_count == Verifiable_SmartData::ITERATIONS_MEASURE)
            //            db<SmartData>(LOGGER)
            //                << "STL_Verifier_Interest[SEU](from=" << it->object()->_interested->unit_dev() << ",to"
            //                << it->object()->_interest->unit_dev()
            //                << ")"
            //                   " ==> SUM of #Iterations("
            //                << Verifiable_SmartData::ITERATIONS_MEASURE << ")=" << ((double)it->object()->_sum) /
            //                1000.
            //                << ",avg=" << (((double)it->object()->_sum) / 1000.) /
            //                Verifiable_SmartData::ITERATIONS_MEASURE
            //                << ",avg_trace_len="
            //                << (((double)it->object()->_sizes)) / Verifiable_SmartData::ITERATIONS_MEASURE << endl;
            //    }
            //    // db<SmartData>(LOGGER) << "SEU HP" << endl;
            //    // for(STL_Verifier_Interest::List::Iterator it = d->_stl_verifiers->begin(); it !=
            //    // d->_stl_verifiers->end(); it++) {
            //    //     if (it->object()->_period > 0) {
            //    //         it->object()->_extended_stl_rule->evaluate(Microsecond(SEU_SmartData::now()));
            //    //     }
            //    //     db<SmartData>(LOGGER) << "SmartData[SEU]::update(i from=" <<
            //    it->object()->_interested->unit_dev()
            //    <<
            //    //     ",to" << it->object()->_interest->unit_dev() << ")" << endl; if (it->object()->result() ==
            //    false) {
            //    //         db<SmartData>(ERR) << "SYSTEM IS UNSAFE! Description:" << endl;
            //    //         db<SmartData>(ERR) << "\tInterest:" << it->object()->_interest->unit_dev() << ",last
            //    sample:
            //    //         time=" << it->object()->_interest->when() << ",mu=" << it->object()->_interest->result() <<
            //    endl;
            //    //         db<SmartData>(ERR) << "\tInterested:" << it->object()->_interested->unit_dev() << ",last
            //    sample:
            //    //         time=" << it->object()->_interested->when() << ",mu=" <<
            //    it->object()->_interested->result() <<
            //    //         endl; db<SmartData>(ERR) << "\tNow:" << SEU_SmartData::now() << ",period=" <<
            //    //         it->object()->_period << endl;
            //    //     };
            //    // }
        }
    }

    friend OStream &operator<<(OStream &db, const SEU_SmartData *d) { // pass-by-reference was creating a copy...
        db << "SmartData[SEU]::state_description={" << "\nsafe? " << d->_safe << "}";
        return db;
    }

    void add_boolean_filter(Boolean_Filter *bf) { _boolean_filters->insert(bf->link()); }
    void remove_boolean_filter(Boolean_Filter *bf) { _boolean_filters->remove(bf->link()); }

    void print_stats() { db<SmartData>(LOGGER) << "SmartData[SEU]::finish!" << endl; }

  private:
    // Network::Observer::update pure virtual method, called whenever the Network receives a SmartData-related
    // message
    void update(typename Network::Observed *obs, Buffer *buffer) {
        db<SmartData>(TRC) << "SmartData[SEU]::update(obs=" << obs << ",buf=" << buffer << ")" << endl;
        Header *header = buffer->frame()->template data<Header>();
        switch (header->type()) {
            case INTEREST: {
                Interest *interest = buffer->frame()->template data<Interest>();
                db<SmartData>(INF) << "SmartData[SEU]::update:msg=" << *interest << endl;
                // bool removed;
                //  if(header->mode() & REVOKE) {
                //      db<SmartData>(INF) << "SmartData[SEU]::Revoke!" << endl;

                //     STL_Verifier_Interest::List::Iterator next;
                //     for(STL_Verifier_Interest::List::Iterator it = _stl_verifiers->begin(); it !=
                //     _stl_verifiers->end(); it++) {
                //         removed = true;
                //         while (it != _stl_verifiers->end() && removed) {
                //             next = it->next();
                //             if (it->object()->interest() == i || it->object()->interested()->unit_dev() == i) {
                //                 removed = true;
                //                 _stl_verifiers->remove(it->object()->link());
                //                 delete it->object();
                //                 it = next; // "it" is broken if we remove the object before incrementing
                //             } else {
                //                 removed = false;
                //             }
                //         }
                //         if (it == _stl_verifiers->end())
                //             break;
                //     }

                //     for(Boolean_Filter::List::Iterator it = _boolean_filters->begin(); it != _boolean_filters->end();
                //     it++) {
                //         it->object()->remove_input(i);
                //     }
                //     return;
                // }
                // db<SmartData>(INF) << "SmartData[SEU]::not Revoke!" << endl;
                const Unit_Dev_Expiry i = Unit_Dev_Expiry(interest->unit(), interest->device(), interest->expiry());
                Verifiable_SmartData *vsd =
                    new Verifiable_SmartData(interest->unit(), interest->device(), interest->expiry(), interest->region());
                Boolean_Filter *bf_left = new MU_Arrival(interest->unit(), interest->device(), interest->expiry(), interest->period());
                bf_left->register_smartdata(vsd);
                vsd->use();
                db<SmartData>(INF) << "SmartData[SEU]::MU_Arrival created for ude=" << i << bf_left << endl;

                bool used                   = false;
                STL_Verifier_Interest *rule = 0;
                // STL_Verifier_Interest::List *stl_verifiers_mising_source = new STL_Verifier_Interest::List();
                for (Boolean_Filter::List::Iterator it = _boolean_filters->begin(); it != _boolean_filters->end(); it++) {
                    rule = 0;
                    used = false;
                    if (it->object()->unit_dev() == i) { // is the bf main SD, so, it will not use it for sure
                        if (it->object()->register_smartdata(vsd)) {
                            vsd->use();
                            db<SmartData>(INF) << "SmartData[SEU]::own value! ude=" << it->object()->unit_dev() << endl;
                        } else {
                            db<SmartData>(INF) << "SmartData[SEU]::already registered! ude=" << it->object()->unit_dev() << endl;
                        }
                        continue;
                    }
                    used = it->object()->add_input(vsd);
                    if (used) {
                        if (it->object()->unit_dev() == Unit_Dev_Expiry(SmartData::Unit::MONITOR, 0, 100000)) { // ignore monitoring
                            db<SmartData>(INF) << "SmartData[SEU]::added to monitor! dev=" << vsd->dev() << endl;
                            continue;
                        } else {
                            db<SmartData>(LOGGER) << "SmartData[SEU]::used! who=" << it->object()->unit_dev() << ",bf_r=" << it->object()
                                                  << ",interested in=" << vsd->unit_dev() << bf_left << endl;
                            rule = new STL_Verifier_Interest(Microsecond(interest->expiry()), it->object()->period(),
                                                             it->object()); // it->object()->period()
                            db<SmartData>(LOGGER) << "SmartData[SEU]::STL verifier created for ude=" << it->object()->unit_dev() << endl;
                            rule->register_boolean_filter_data_source(bf_left);
                            db<SmartData>(LOGGER) << "SmartData[SEU]::inserting into _stl_verifiers!" << endl;
                            _stl_verifiers->insert(rule->link());
                            rule->attach(this);
                        }
                    }
                }
                db<SmartData>(INF) << "SmartData[SEU]::out of loop!" << endl;

                vsd->remove();
                db<SmartData>(INF) << "SmartData[SEU]::after remove!" << endl;
                if (!vsd->in_use()) {
                    //db<SmartData>(LOGGER) << "SmartData[SEU]::before ignoring this one -- for some reason it was not used!" << endl;
                    //vsd->detach(bf_left);
                    //delete bf_left;
                    //db<SmartData>(LOGGER) << "SmartData[SEU]::deleting unused SD!" << endl;
                    //delete vsd;
                } else {
                    if (Traits<Build>::cloud) {
#ifdef CLOUD_INTEGRATION
                        db<SmartData>(INF) << "SmartData[SEU]::adding to devices!" << endl;
                        _devices->insert(vsd->new_link());
                        db<SmartData>(INF) << "SmartData[SEU]::after adding to devices!" << endl;
                        vsd->create_iot_series();
#endif
                    }
                }
                db<SmartData>(INF) << "SmartData[SEU]::finished this one!" << endl;
            } break;
            case RESPONSE: {
                Response *response = buffer->frame()->template data<Response>();
                _start             = true;
                db<SmartData>(INF) << "SmartData[SEU]::update:msg=" << response->origin() << "d=" << response->device() << endl;
                if (Traits<Build>::cloud) {
#ifdef CLOUD_INTEGRATION
                    if (response->device() == 16) {
                        for (Verifiable_SmartData::List::Iterator it = _devices->begin(); it != _devices->end(); it++) {
                            Motion_Vector m = response->template value<Motion_Vector>();
                            it->object()->set_mv(m);
                        }
                    }
#endif
                    // There is no need, if we can map all interest relations we already cover all relevant smartdata
                    // being generated Is there a need to check data that is not of interest to someone? Is it even
                    // useful to the system?
                }

            } break;
            case COMMAND: {
                Command *command = buffer->frame()->template data<Command>();
                db<SmartData>(INF) << "SmartData[SEU]::update:msg=" << *command << endl;
                db<SmartData>(ERR) << "CMD--SmartData[SEU]::update: not commanded!" << endl;
            } break;
            case CONTROL: {
                Control *control = buffer->frame()->template data<Control>();
                db<SmartData>(INF) << "CTRL--SmartData[SEU]::update:msg=" << *control << endl;
                db<SmartData>(ERR) << "CTRL--SmartData[SEU]::update: not controlled!" << endl;
            } break;
        }
    }

    void update(STL_Verifier_Interest::Observed *obs) {
        STL_Verifier_Interest *i = reinterpret_cast<STL_Verifier_Interest *>(obs);
        db<SmartData>(TRC) << "SmartData[SEU]::update(from=" << i->_interested->unit_dev() << ",to" << i->_interest->unit_dev() << ")"
                           << endl;

        if (i->result() == false) {
            db<SmartData>(ERR) << "SYSTEM IS UNSAFE! Description:" << endl;
            i->_interested->description();
            db<SmartData>(ERR) << "\tInterest:" << i->_interest->unit_dev() << ",last sample: time=" << i->_interest->_time
                               << ",mu=" << i->_interest->_truth_value << ",elapsed=" << i->_interest->_elapsed << endl;
            db<SmartData>(ERR) << "\tInterested:" << i->_interested->unit_dev() << ",last sample: time=" << i->_interested->_time
                               << ",mu=" << i->_interested->_truth_value << ",elapsed=" << i->_interested->_elapsed << endl;
            db<SmartData>(ERR) << "\tNow:" << SEU_SmartData::now() << ",period=" << i->_period << endl;
        }
    }

  private:
    bool _safe;
    Boolean_Filter::List *_boolean_filters;
    STL_Verifier_Interest::List *_stl_verifiers; // needed to handle revoke
    Periodic_Thread *_thread;
    Verifiable_SmartData::List *_devices;
    bool _start;
    Microsecond _period;
};

bool Verifiable_SmartData::notify(Verifiable_SmartData *d) {
    db<Observers>(TRC) << "Data_Observed::notify(this=" << this << ")" << endl;
    bool notified = false;
    for (Iterator i = _observers.begin(); i != _observers.end(); i++) {
        db<Observers>(INF) << "Observed::notify(this=" << this << ",obs=" << i->object() << ")" << endl;
        i->object()->update(d);
        notified = true;
    }
    return notified;
}

void Verifiable_SmartData::attach(Observer *o) { _observers.insert(o->new_link()); }
void Verifiable_SmartData::detach(Observer *o) { _observers.remove(o); }

bool Boolean_Filter::notify(Boolean_Filter *d) {
    db<Observers>(TRC) << "Data_Observed::notify(this=" << this << ")" << endl;
    bool notified = false;
    for (Iterator i = _observers.begin(); i != _observers.end(); i++) {
        db<Observers>(INF) << "Boolean_Filter::notify(this=" << this << ",obs=" << i->object() << ")" << endl;
        i->object()->update(d);
        notified = true;
    }
    return notified;
}

void Boolean_Filter::attach(Observer *o) { _observers.insert(o->new_link()); }
void Boolean_Filter::detach(Observer *o) { _observers.remove(o); }

void *STL_Verifier_Interest::evaluate(void *p) {
    STL_Verifier_Interest *i = reinterpret_cast<STL_Verifier_Interest *>(p);
    while (!i->_start) {
        Thread::yield();
    }
    Periodic_Thread::wait((TSC::time_stamp() + (8 * i->_period) / 10)); // add 80% of period
    SmartData::Time start;
    while (true) {
        start = nanos();
        i->_extended_stl_rule->evaluate(Microsecond(SEU_SmartData::now()));
        db<SmartData>(TRC) << "STL_Verifier_Interest[SEU]::result=" << i->result() << "}" << endl;
        i->notify(); // SEU will decide what to do
        if (i->_count < Verifiable_SmartData::ITERATIONS_MEASURE) {
            i->_sum = i->_sum + (nanos() - start);
            i->_sizes += i->_extended_stl_rule->size();
        } else {
			//static QUARK::Mutex lock;
			//lock.acquire();
			//kout << "STL_Verifier_Interest[SEU](from=" << i->_interested->unit_dev() << ",to" << i->_interest->unit_dev() << ")"" ==> SUM of #Iterations(" << Verifiable_SmartData::ITERATIONS_MEASURE << ")=" << ((double)i->_sum)/1000. << ",avg=" << (((double)i->_sum)/1000.)/Verifiable_SmartData::ITERATIONS_MEASURE << ",avg_trace_len=" << (((double)i->_sizes))/Verifiable_SmartData::ITERATIONS_MEASURE << endl;
			//lock.release();
		}
        i->_count++;
        Periodic_Thread::wait();
    }
}

#endif
