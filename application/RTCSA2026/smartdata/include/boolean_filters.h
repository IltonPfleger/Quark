#include <seu.h>
#include <utility/math.h>

class Monitoring : public Boolean_Filter {
  public:
    Monitoring(Unit_Dev_Expiry::List *supported_SmartData, Microsecond period = 0)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::MONITOR, 0, 100000), period) {
        db<SmartData>(TRC) << "SmartData[SEU]::Monitor::this=" << this << endl;
    }
    ~Monitoring() {}

  private:
    virtual bool evaluate() {
        db<SmartData>(TRC) << "Monitoring[SEU]::evaluate()::this=" << this << endl;
        bool plausibility = true;
        Microsecond t     = 0;
        for (Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
            plausibility &= !it->object()->expired();
        }
        if (_SmartData != 0) {
            plausibility &= _SmartData->expired();
            t = Microsecond(_SmartData->origin_time());
        }
        _truth_value = plausibility;
        _time        = t;
        return true;
    }

    virtual void description() const {
        // db << "Monitor((u,d)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ",origin=" <<
        //     it->object()->origin_time() << ")";
        // }
        // db << "}";
    }
};

class Environment_Min_Max_Temperature_Boolean_Filter : public Boolean_Filter, SmartData {
  public:
    Environment_Min_Max_Temperature_Boolean_Filter(Unit_Dev_Expiry::List *supported_SmartData,
                                                   UInt32 dev,
                                                   Time expiry,
                                                   Microsecond period = 0)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::Temperature, dev, expiry), period) {
        db<SmartData>(TRC) << "SmartData[SEU]::Environment_Min_Max_Temperature_Boolean_Filter::this=" << this << endl;
    }
    ~Environment_Min_Max_Temperature_Boolean_Filter() {}

  private:
    virtual bool evaluate() {
        db<SmartData>(TRC) << "Environment_Min_Max_Temperature_Boolean_Filter[SEU]::evaluate()::this=" << this
                           << ",_inputs->size()=" << _inputs->size() << ",head=" << _inputs->head()
                           << ",tail=" << _inputs->tail() << endl;
        bool plausibility = true;
        Microsecond t     = 0;
        for (Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
            db<SmartData>(INF) << "Environment_Min_Max_Temperature_Boolean_Filter[SEU]::evaluate()::value="
                               << *(it->object()->value<float>()) << ",it=" << it << ",next=" << it->next() << endl;
            plausibility &=
                *(it->object()->value<float>()) > 183.15; // > 90 oC (record lowest temperature on earth is -89.2)
            plausibility &=
                *(it->object()->value<float>()) < 333.15; // < 60 oC (record highest temperature on earth is 56.7)
            if (it->object()->origin_time() > t) {
                t = Microsecond(it->object()->origin_time());
            }
        }
        if (_SmartData != 0) {
            plausibility &= _SmartData->expired();
            t = Microsecond(_SmartData->origin_time());
        }

        _truth_value = plausibility;
        _time        = t;
        return true;
    }

    virtual void description() const {
        // db << "Environment_Min_Max_Temperature_Boolean_Filter((u,d)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")=" <<
        //     *(it->object()->value<float>()) << ")";
        // }
        // db << "}";
    }
};

class Dummy_Speed_Boolean_Filter : public Boolean_Filter, SmartData {
  public:
    Dummy_Speed_Boolean_Filter(Unit_Dev_Expiry::List *supported_SmartData, UInt32 dev, Time expiry)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::Speed, dev, expiry)) {
        db<SmartData>(TRC) << "SmartData[SEU]::Dummy_Speed_Boolean_Filter::this=" << this << endl;
    }
    ~Dummy_Speed_Boolean_Filter() {}

  private:
    virtual bool evaluate() {
        bool plausibility = true;
        Microsecond t     = 0;

        for (Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
            plausibility &= *(it->object()->value<int>()) < 56; // 200 kmph limit
            if (it->object()->origin_time() > t) {
                t = Microsecond(it->object()->origin_time());
            }
        }
        if (_SmartData != 0) {
            plausibility &= _SmartData->expired();
            t = Microsecond(_SmartData->origin_time());
        }

        _truth_value = plausibility;
        _time        = t;
        return true;
    }

    virtual void description() const {
        // db << "Dummy_Speed_Boolean_Filter((u,d)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")=" <<
        //     *(it->object()->value<int>()) << ")";
        // }
        // db << "}";
    }
};

class Dummy_Camera_Boolean_Filter : public Boolean_Filter, SmartData {
  public:
    Dummy_Camera_Boolean_Filter(Unit_Dev_Expiry::List *supported_SmartData, UInt32 dev, Time expiry)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::RAW_BGR, dev, expiry)) {
        db<SmartData>(TRC) << "SmartData[SEU]::Dummy_Camera_Boolean_Filter::this=" << this << endl;
    }
    ~Dummy_Camera_Boolean_Filter() {}

  private:
    virtual bool evaluate() {
        bool plausibility = true;
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     plausibility &= *(it->object()->value<unsigned char>()) < 56;
        // }
        _truth_value = plausibility;
        if (_SmartData != 0)
            _time = Microsecond(_SmartData->origin_time());
        else
            _time = Microsecond(TSC::time_stamp());
        return true;
    }

    virtual void description() const {
        // db << "Dummy_Camera_Boolean_Filter((u,d)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")=(image is too big to print?),
        //     showing expired() instead=" << it->object()->expired() << ")";
        // }
        // db << "}";
    }
};

class Dummy_Direction_Boolean_Filter : public Boolean_Filter, SmartData {
  public:
    Dummy_Direction_Boolean_Filter(Unit_Dev_Expiry::List *supported_SmartData, UInt32 dev, Time expiry)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::RAW_BGR, dev, expiry)) {
        db<SmartData>(TRC) << "SmartData[SEU]::Dummy_Direction_Boolean_Filter::this=" << this << endl;
    }
    ~Dummy_Direction_Boolean_Filter() {}

  private:
    virtual bool evaluate() {
        bool plausibility = true;
        Microsecond t     = 0;
        for (Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
            plausibility &= *(it->object()->value<char>()) - '0' < 4 &&
                            *(it->object()->value<char>()) - '0' >= 0; // enum from 0 to 3
            if (it->object()->origin_time() > t) {
                t = Microsecond(it->object()->origin_time());
            }
        }
        if (_SmartData != 0) {
            plausibility &= _SmartData->expired();
            t = Microsecond(_SmartData->origin_time());
        }

        _truth_value = plausibility;
        _time        = t;
        return true;
    }

    virtual void description() const {
        // db << "Dummy_Direction_Boolean_Filter((u,d)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")=" <<
        //     *(it->object()->value<char>()) << ")";
        // }
        // db << "}";
    }
};

template <bool THROTTLE> class Dummy_Acceleration_Boolean_Filter : public Boolean_Filter, SmartData {
  private:
    static const int MAXIMUM_ACCELERATION = 10; // ms2
    static const int MAXIMUM_BRAKE        = 10; // ms2
  public:
    Dummy_Acceleration_Boolean_Filter(Unit_Dev_Expiry::List *supported_SmartData, UInt32 dev, Time expiry)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::Acceleration, dev, expiry)) {
        db<SmartData>(TRC) << "SmartData[SEU]::Dummy_Acceleration_Boolean_Filter::this=" << this << endl;
    }
    ~Dummy_Acceleration_Boolean_Filter() {}

  private:
    virtual bool evaluate() {
        bool plausibility = true;
        Microsecond t     = 0;
        for (Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
            plausibility &= *(it->object()->value<int>()) < (int)((THROTTLE ? MAXIMUM_ACCELERATION : MAXIMUM_BRAKE));
            plausibility &= *(it->object()->value<int>()) > 0;
            if (it->object()->origin_time() > t) {
                t = Microsecond(it->object()->origin_time());
            }
        }
        if (_SmartData != 0) {
            plausibility &= _SmartData->expired();
            t = Microsecond(_SmartData->origin_time());
        }

        _truth_value = plausibility;
        _time        = t;
        return true;
    }

    virtual void description() const {
        // db << "Dummy_Acceleration_Boolean_Filter((u,d)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")=" <<
        //     *(it->object()->value<int>()) << ")";
        // }
        // db << "}";
    }
};

typedef Dummy_Acceleration_Boolean_Filter<true> Dummy_Throttle_Boolean_Filter;
typedef Dummy_Acceleration_Boolean_Filter<false> Dummy_Brake_Boolean_Filter;

class Dummy_Steer_Angle_Boolean_Filter : public Boolean_Filter, SmartData {
  public:
    Dummy_Steer_Angle_Boolean_Filter(Unit_Dev_Expiry::List *supported_SmartData, UInt32 dev, Time expiry)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::Angle, dev, expiry)) {
        db<SmartData>(TRC) << "SmartData[SEU]::Dummy_Steer_Angle_Boolean_Filter::this=" << this << endl;
    }
    ~Dummy_Steer_Angle_Boolean_Filter() {}

  private:
    virtual bool evaluate() {
        bool plausibility = true;
        Microsecond t     = 0;
        for (Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
            plausibility &= *(it->object()->value<int>()) < (int)(1.5708); // steering limit of 90 degrees?
            if (it->object()->origin_time() > t) {
                t = Microsecond(it->object()->origin_time());
            }
        }
        if (_SmartData != 0) {
            plausibility &= _SmartData->expired();
            t = Microsecond(_SmartData->origin_time());
        }

        _truth_value = plausibility;
        _time        = t;
        return true;
    }

    virtual void description() const {
        // db << "Dummy_Steer_Angle_Boolean_Filter((u,d)={";
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")=" <<
        //     *(it->object()->value<int>()) << ")";
        // }
        // db << "}";
    }
};

// TODO
class Dummy_GPS_Boolean_Filter : public Boolean_Filter, SmartData {
  public:
    Dummy_GPS_Boolean_Filter(Unit_Dev_Expiry::List *supported_SmartData, UInt32 dev, Time expiry)
        : Boolean_Filter(supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::GPS3I, dev, expiry)) {
        db<SmartData>(TRC) << "SmartData[SEU]::Dummy_GPS_Boolean_Filter::this=" << this << endl;
    }
    ~Dummy_GPS_Boolean_Filter() {}

  private:
    virtual bool evaluate() {
        bool plausibility = true;
        Microsecond t     = 0;
        // Point<int, 3> * pos;
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     pos = it->object()->value<Point<int, 3>>()));
        //     plausibility &= p.x < x_lim * INTEGER_ARITHMETIC_PRECISION;
        //     plausibility &= p.y < y_lim * INTEGER_ARITHMETIC_PRECISION;
        //     plausibility &= p.z < z_lim * INTEGER_ARITHMETIC_PRECISION;
        // }
        _truth_value = plausibility;
        if (_SmartData != 0) {
            plausibility &= _SmartData->expired();
            t = Microsecond(_SmartData->origin_time());
        }

        _truth_value = plausibility;
        _time        = t;
        return true;
    }

    virtual void description() const {
        // db << "Dummy_GPS_Boolean_Filter((u,d)={";
        // Point<int, 3> * p;
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++) {
        //     p = it->object()->value<Point<int, 3>>();
        //     db << "(u=" << it->object()->unit() << ",d=" << it->object()->dev() << ")=";
        //     db << ((double) (p->x())) << "," << ((double) (p->y())) << "," << ((double) (p->z())) << ")";
        // }
        // db << "}";
    }
};

class Road_Parameters {
  public:
    Road_Parameters(float max_acc, float max_brk, float min_brk, float max_lat_acc, float min_lat_brk)
        : maximum_acceleration(max_acc),
          maximum_brakage(max_brk),
          minimum_brakage(min_brk),
          maximum_lat_acceleration(max_lat_acc),
          minimum_lat_brakage(min_lat_brk) {}

    void set_default() {
        // Source https://intel.github.io/ad-rss-lib/ad_rss/Appendix-ParameterDiscussion/
        maximum_acceleration = 3.5; // m/s2
        maximum_brakage      = 8;   // m/s2
        minimum_brakage      = 4;   // m/s2

        maximum_lat_acceleration = 0.2; // m/s2
        minimum_lat_brakage      = 0.8; // m/s2
        mu                       = 0.1; // m
    }

  public:
    Float32 maximum_acceleration;
    Float32 maximum_brakage;
    Float32 minimum_brakage;

    Float32 maximum_lat_acceleration;
    Float32 minimum_lat_brakage;
    Float32 mu;
};

class RSS_Safe_Distance : public Boolean_Filter, SmartData {

  public:
    RSS_Safe_Distance(Unit_Dev_Expiry::List *supported_SmartData,
                      Road_Parameters *rp_ego,
                      Road_Parameters *rp_other,
                      Microsecond rho)
        : Boolean_Filter(
              supported_SmartData, Unit_Dev_Expiry(SmartData::Unit::SAFETY_MODEL, 0, SmartData::Time(rho)), rho),
          _rp_ego(rp_ego),
          _rp_other(rp_other),
          _response_time(rho),
          _last_consumption(0) {}
    ~RSS_Safe_Distance() {}

    virtual bool evaluate() {

        Motion_Vector *ego_reference = 0;
        Motion_Vector *temp_mv       = 0;
        Motion_Vector *mvs           = 0;
        Motion_Vector *to_check      = 0;
        db<SmartData>(ERR) << "RSS::evaluate" << endl;
        for (Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end();
             it++) { // we still need to add road boundaries
            if (it->object()->origin_time() == 0) return false;
            db<SmartData>(INF) << "RSS:" << it->object()->unit_dev() << endl;

            if (it->object()->unit() ==
                SmartData::Unit(SmartData::Unit::MOTION_VECTOR_LOCAL)) { // forces to use Unit operator==
                temp_mv = it->object()->value<Motion_Vector>();
                if (temp_mv->_valid) {
                    if (temp_mv->_obj_class == Traits<Object_Classes>::REFERENCE_CLASS) {
                        ego_reference = temp_mv;
                    } else if (temp_mv->_obj_class == Traits<Object_Classes>::EGO_CLASS ||
                               temp_mv->_obj_class == Traits<Object_Classes>::EGO_FUTURE_CLASS ||
                               temp_mv->_obj_class == Traits<Object_Classes>::EGO_DESTINATION_CLASS) {
                        continue; // skip non-object
                    } else {
                        mvs = temp_mv;
                    }
                }
            }
        }
        SmartData::Time start = nanos();
        _last_consumption     = TSC::time_stamp();
        db<SmartData>(INF) << "RSS::evaluate, t=" << _last_consumption << endl;

        bool valid = false;
        UInt32 i   = 0;

        _lateral_safe      = true;
        _longitudinal_safe = true;
        if (ego_reference) {
            if (mvs) {
                do {
                    to_check = mvs++;
                    check_object(ego_reference, to_check);
                    i++;
                    valid = mvs->_valid;
                } while (valid && _lateral_safe && _longitudinal_safe);

                _truth_value = _lateral_safe && _longitudinal_safe;
                _time        = ego_reference->_timestamp;
            } else {
                _truth_value = true;
                _time        = ego_reference->_timestamp;
            }
        } else {
            _truth_value = false;
            _time        = 0;
            _sum         = _sum + (nanos() - start);
            if (_count == Verifiable_SmartData::ITERATIONS_MEASURE) {
                db<SmartData>(LOGGER) << "RSSlon&lat ==> SUM of " << Verifiable_SmartData::ITERATIONS_MEASURE << "="
                                      << ((double)_sum) / 1000.
                                      << ",avg=" << (((double)_sum) / 1000.) / Verifiable_SmartData::ITERATIONS_MEASURE
                                      << endl;
            }
            _count++;

            return false;
        }
        _sum = _sum + (nanos() - start);
        if (_count == Verifiable_SmartData::ITERATIONS_MEASURE) {
            db<SmartData>(LOGGER) << "RSSlon&lat ==> SUM of " << Verifiable_SmartData::ITERATIONS_MEASURE << "="
                                  << ((double)_sum) / 1000.
                                  << ",avg=" << (((double)_sum) / 1000.) / Verifiable_SmartData::ITERATIONS_MEASURE
                                  << endl;
        }
        _count++;
        return true;
    }

    virtual void description() const {
        db<SmartData>(LOGGER) << "RSS_Boolean_Filter(={safe_lateral=" << _lateral_safe
                              << ",safe_longitudinal=" << _longitudinal_safe << endl;
        // for(Verifiable_SmartData::List::Iterator it = _inputs->begin(); it != _inputs->end(); it++)
        //     it->object()->log_state();
    }

  private:
    void check_object(Motion_Vector *ego, Motion_Vector *other) {
        Float32 dmin;
        Float32 other_speed = other->_speed + ego->_speed;

        bool static_obj = other->_obj_class == Traits<Object_Classes>::UNKNOWN_CLASS;

        Float64 heading     = ego->_heading * 180 / Math::pi();
        bool other_in_front = false;
        bool other_to_right = false;
        float lat_dist      = 0;
        float lon_dist      = 0;

        if (heading < 45 || heading > 315) {           // E
            other_in_front = other->_location[0] >= 0; // going East, other in front whenever other x distance is > 0
            other_to_right = other->_location[1] <= 0; // going East, other in front whenever other y distance is < 0
            lat_dist       = other->_location[1];
            if (other_to_right) lat_dist *= -1;
            lon_dist = other->_location[0];
            if (!other_in_front) lon_dist *= -1;
        } else if (heading < 125 && heading > 45) {    // N
            other_in_front = other->_location[1] >= 0; // going North, other in front whenever other y distance is > 0
            other_to_right = other->_location[0] >= 0; // going North, other in right whenever other x distance is > 0
            lat_dist       = other->_location[0];
            if (!other_to_right) lat_dist *= -1;
            lon_dist = other->_location[1];
            if (!other_in_front) lon_dist *= -1;
        } else if (heading > 125 && heading < 225) {   // W
            other_in_front = other->_location[0] <= 0; // going West, other in front whenever other x distance is < 0
            other_to_right = other->_location[1] >= 0; // going West, other in right whenever other y distance is > 0
            lat_dist       = other->_location[1];
            if (!other_to_right) lat_dist *= -1;
            lon_dist = other->_location[0];
            if (other_in_front) lon_dist *= -1;
        } else if (heading > 225 && heading < 315) {   // S
            other_in_front = other->_location[1] <= 0; // going South, other in front whenever other y distance is < 0
            other_to_right = other->_location[0] <= 0; // going South, other in right whenever other x distance is < 0
            lat_dist       = other->_location[0];
            if (other_to_right) lat_dist *= -1;
            lon_dist = other->_location[1];
            if (other_in_front) lon_dist *= -1;
        }
        lat_dist = lat_dist * 100; // cm
        lon_dist = lon_dist * 100; // cm

        db<SmartData>(TRC) << "LonDist:" << lon_dist << ",LatDist=" << lat_dist << ",heading=" << heading
                           << ",otherisinfront=" << other_in_front << ",othertoright=" << other_to_right
                           << ",ego_s=" << ego->_speed << ",other_s" << other_speed << endl;

        if (!other_in_front) { // i am in front -- no need for longitudinal verification
            if (lon_dist <= Traits<Object_Classes>::PASSENGER_CAR_LENGTH /
                                2) { // if in danger laterally (e.g., they can hit laterally)
                if (other_to_right) {
                    dmin = minimum_lateral_distance(ego->_speed * Math::cos(other->_heading),
                                                    other_speed * Math::cos(other->_heading), static_obj, true);
                    db<SmartData>(INF) << "dmin_lat=" << dmin << endl;
                    _lateral_safe &= lat_dist >= dmin * 100 + Traits<Object_Classes>::PASSENGER_CAR_WIDTH / 2; // cm
                } else {
                    dmin = minimum_lateral_distance(other_speed * Math::cos(other->_heading),
                                                    ego->_speed * Math::cos(other->_heading), static_obj, false);
                    db<SmartData>(INF) << "dmin_lat=" << dmin << endl;
                    _lateral_safe &= lat_dist >= dmin * 100 + Traits<Object_Classes>::PASSENGER_CAR_WIDTH / 2; // cm
                }
            }
        } else {
            if (lat_dist <= Traits<Object_Classes>::PASSENGER_CAR_WIDTH /
                                2) { // if in dange longitudinally (e.g., they are close laterally)
                dmin = minimum_frontal_distance(ego->_speed * Math::sin(other->_heading),
                                                other_speed * Math::sin(other->_heading), static_obj);
                db<SmartData>(INF) << "dmin_lon=" << dmin * 100 << "+"
                                   << Traits<Object_Classes>::PASSENGER_CAR_LENGTH / 2 << ",d" << lon_dist << endl;
                _longitudinal_safe &= lon_dist >= dmin * 100 + Traits<Object_Classes>::PASSENGER_CAR_LENGTH / 2; // cm
            }
            if (lon_dist <= Traits<Object_Classes>::PASSENGER_CAR_LENGTH /
                                2) { // if in danger laterally (e.g., they can hit laterally)
                if (other_to_right) {
                    dmin = minimum_lateral_distance(ego->_speed * Math::cos(other->_heading),
                                                    other_speed * Math::cos(other->_heading), static_obj, true);
                    db<SmartData>(INF) << "dmin_lat=" << dmin << endl;
                    _lateral_safe &= lat_dist >= dmin * 100 + Traits<Object_Classes>::PASSENGER_CAR_WIDTH / 2; // cm
                } else {
                    dmin = minimum_lateral_distance(other_speed * Math::cos(other->_heading),
                                                    ego->_speed * Math::cos(other->_heading), static_obj, false);
                    db<SmartData>(INF) << "dmin_lat=" << dmin << endl;
                    _lateral_safe &= lat_dist >= dmin * 100 + Traits<Object_Classes>::PASSENGER_CAR_WIDTH / 2; // cm
                }
            }
        }
    }

    float minimum_frontal_distance(float vr, float vf, bool static_obj) {
        float dmin = 0;
        float rho  = ((float)_response_time) / 1000000.;
        if (static_obj) {
            dmin = vr * rho + (_rp_ego->maximum_acceleration * rho * rho) / 2 +
                   ((vr + rho * _rp_ego->maximum_acceleration) * (vr + rho * _rp_ego->maximum_acceleration)) /
                       (2 * _rp_ego->minimum_brakage) -
                   0;
        } else {
            dmin = vr * rho + (_rp_ego->maximum_acceleration * rho * rho) / 2 +
                   ((vr + rho * _rp_ego->maximum_acceleration) * (vr + rho * _rp_ego->maximum_acceleration)) /
                       (2 * _rp_ego->minimum_brakage) -
                   (vf * vf) / (2 * _rp_other->maximum_brakage);
        }
        dmin = dmin > 0 ? dmin : 0;
        return dmin;
    }

    float
    minimum_lateral_distance(float v1, float v2, bool static_obj, bool ego_left = true /*, float l, float lout*/) {
        float dmin;

        Road_Parameters *_rp_left;
        Road_Parameters *_rp_right;
        if (ego_left) {
            _rp_left  = _rp_ego;
            _rp_right = _rp_other;
        } else {
            _rp_left  = _rp_other;
            _rp_right = _rp_ego;
        }

        float rho = ((float)_response_time) / 1000000.;

        if (static_obj) {
            float v1p = v1 + rho * _rp_left->maximum_lat_acceleration;

            dmin = (rho * (v1 + v1p) / 2) + ((v1p * v1p) / (2 * _rp_left->minimum_lat_brakage)) - 0;
        } else {
            float v1p = v1 + rho * _rp_left->maximum_lat_acceleration;
            float v2p = v2 + rho * _rp_right->maximum_lat_acceleration;

            dmin = (rho * (v1 + v1p) / 2) + ((v1p * v1p) / (2 * _rp_left->minimum_lat_brakage)) -
                   ((rho * (v2 + v2p) / 2) + ((v2p * v2p) / (2 * _rp_right->minimum_lat_brakage)));
        }

        // dmin = dmin + mu_lateral_velocity(l, lout);
        dmin = dmin > 0 ? dmin : -dmin; // we need to address both left and right reponses
        return dmin + _rp_ego->mu;
    }

    SmartData::Time nanos() {
        SmartData::Time us = (unsigned long)QUARK::Microsecond(QUARK::Timer::now());
        return us;
    }

    bool in_need_for_lateral_proper_response() { return !_lateral_safe; }
    bool in_need_for_longitudinal_proper_response() { return !_longitudinal_safe; }

  private:
    bool _lateral_safe;
    bool _longitudinal_safe;
    Road_Parameters *_rp_ego;
    Road_Parameters *_rp_other;
    Microsecond _response_time;
    SmartData::Time _last_consumption;
    UInt32 _count;
    SmartData::Time _sum;
};
