#pragma once
// EPOS SmartData Declarations
//
// Smart Data encapsulates Transducers (i.e. sensors and actuators), local or remote, and bridges them with Network
// Transducers must be Observed objects, must implement either sense() or actuate(), and must define UNIT, NUM, and
// UNCERTAINTY.

#include "system/types.h"
#include <memory/Heap.hpp>
#include <system/meta.h>
#include <utility/geometry.h>
#include <utility/math.h>
#include <utility/observer.h>
#include <utility/predictor.h>

#define __UTIL

// Scale for Geographic Space used by communication protocols (payloadlications always get CM_32)
namespace Space_Time {
enum Scale { _NULL = 0, CMx50_8 = 1, CM_16 = 2, CM_32 = 3 };

template <Scale S> struct Select_Scale {
    using Number =
        typename SWITCH<S, CASE<_NULL, Int8, CASE<CMx50_8, Int8, CASE<CM_16, Int16, CASE<CM_32, Int32>>>>>::Result;
    using Unsigned_Number =
        typename SWITCH<S, CASE<_NULL, UInt8, CASE<CMx50_8, UInt8, CASE<CM_16, UInt16, CASE<CM_32, UInt32>>>>>::Result;

    static const UInt32 PADDING = (S == CMx50_8) ? 8 : (S == CM_16) ? 16 : 0;
};

// Scalable Geographic Space (expressed in m from the center of the Earth; compressible by communication protocols; see
// https://en.wikipedia.org/wiki/Earth-centered,_Earth-fixed_coordinate_system)
template <Scale S>
class _Space : public Point<typename Select_Scale<S>::Number, 3>, private Padding<Select_Scale<S>::PADDING> {
  public:
    using Number          = typename Select_Scale<S>::Number;
    using Unsigned_Number = typename Select_Scale<S>::Unsigned_Number;

    static const Number UNKNOWN = 1 << (sizeof(Number) * 8 - 1);

  public:
    _Space() {}
    _Space(const Number &x, const Number &y, const Number &z)
        : Point<Number, 3>(x, y, z) {}
    _Space(const Point<Number, 3> &p)
        : Point<Number, 3>(p.x(), p.y(), p.z()) {}

    operator _Space<_NULL>() const;
    operator _Space<CMx50_8>() const;
    operator _Space<CM_16>() const;
    operator _Space<CM_32>() const;
} __attribute__((packed));

template <> class _Space<_NULL> {
  public:
    using Number   = typename Select_Scale<_NULL>::Number;
    using Distance = typename Select_Scale<_NULL>::Unsigned_Number;

    static const Number UNKNOWN = 1 << (sizeof(Number) * 8 - 1);

  public:
    _Space() {}
    _Space(const Number &x, const Number &y, const Number &z) {}
    _Space(const Point<Number, 3> &p) {}

    Number x() const { return 0; }
    Number y() const { return 0; }
    Number z() const { return 0; }

    operator _Space<CMx50_8>() const { return _Space<CMx50_8>(0, 0, 0); };
    operator _Space<CM_16>() const { return _Space<CM_16>(0, 0, 0); };
    operator _Space<CM_32>() const { return _Space<CM_32>(0, 0, 0); };

    bool operator==(const _Space<_NULL> &s) const { return true; }
    bool operator!=(const _Space<_NULL> &s) const { return false; }

    Distance operator+(const _Space<_NULL> &s) const { return 0; }
    Distance operator-(const _Space<_NULL> &s) const { return 0; }
    _Space<_NULL> &operator+=(const _Space<_NULL> &p) { return *this; }
    _Space<_NULL> &operator-=(const _Space<_NULL> &p) { return *this; }

    static _Space<_NULL> trilaterate(const _Space<_NULL> &s1,
                                     const Distance &d1,
                                     const _Space<_NULL> &s2,
                                     const Distance &d2,
                                     const _Space<_NULL> &s3,
                                     const Distance &d3) {
        return _Space<_NULL>(0, 0, 0);
    }

    friend OStream &operator<<(OStream &os, const _Space<_NULL> &s) { return os; }

} __attribute__((packed));

template <> inline _Space<CMx50_8>::operator _Space<_NULL>() const {
    return _Space<_NULL>(Point<Number, 3>::x() * 50, Point<Number, 3>::y() * 50, Point<Number, 3>::z() * 50);
}
template <> inline _Space<CMx50_8>::operator _Space<CM_16>() const {
    return _Space<CM_16>(Point<Number, 3>::x() * 50, Point<Number, 3>::y() * 50, Point<Number, 3>::z() * 50);
}
template <> inline _Space<CMx50_8>::operator _Space<CM_32>() const {
    return _Space<CM_32>(Point<Number, 3>::x() * 50, Point<Number, 3>::y() * 50, Point<Number, 3>::z() * 50);
}

template <> inline _Space<CM_16>::operator _Space<_NULL>() const {
    return _Space<_NULL>(Point<Number, 3>::x(), Point<Number, 3>::y(), Point<Number, 3>::z());
}
template <> inline _Space<CM_16>::operator _Space<CMx50_8>() const {
    return _Space<CMx50_8>(Point<Number, 3>::x() / 50, Point<Number, 3>::y() / 50, Point<Number, 3>::z() / 50);
}
template <> inline _Space<CM_16>::operator _Space<CM_32>() const {
    return _Space<CM_32>(Point<Number, 3>::x(), Point<Number, 3>::y(), Point<Number, 3>::z());
}

template <> inline _Space<CM_32>::operator _Space<_NULL>() const {
    return _Space<_NULL>(Point<Number, 3>::x(), Point<Number, 3>::y(), Point<Number, 3>::z());
}
template <> inline _Space<CM_32>::operator _Space<CMx50_8>() const {
    return _Space<CMx50_8>(Point<Number, 3>::x() / 50, Point<Number, 3>::y() / 50, Point<Number, 3>::z() / 50);
}
template <> inline _Space<CM_32>::operator _Space<CM_16>() const {
    return _Space<CM_16>(Point<Number, 3>::x(), Point<Number, 3>::y(), Point<Number, 3>::z());
}

// Time (expressed in us)
class Time {
  public:
    typedef UInt64 Type;

  public:
    Time() {};
    Time(const Infinity &)
        : _time(INFINITE) {};
    Time(const Type &time)
        : _time(time) {};

    void operator=(const Type &time) { _time = time; }
    void operator=(const Type &time) volatile { _time = time; }

    operator Type() { return _time; }
    operator Type() const { return _time; }
    operator Type() volatile { return _time; }

  private:
    Type _time;
} __attribute__((packed));

class Short_Time {
  public:
    typedef Int32 Type;

  public:
    Short_Time() {};
    Short_Time(const Infinity32 &)
        : _time(INFINITE32) {};
    Short_Time(const Type &time)
        : _time(time) {};

    void operator=(const Type &time) { _time = time; }
    void operator=(const Type &time) volatile { _time = time; }

    operator Type() { return _time; }
    operator Type() const { return _time; }
    operator Type() volatile { return _time; }

  private:
    Type _time;
} __attribute__((packed));
typedef Short_Time Time_Offset;

struct Time_Interval {
    Time_Interval(const Time &begin, const Time &end)
        : t0(begin),
          t1(end) {}

    bool contains(const Time &t) const { return (t >= t0) && (t <= t1); }

    Time t0;
    Time t1;
} __attribute__((packed));

template <Scale S> struct _Spacetime {
    using Space  = _Space<S>;
    using Number = typename Space::Number;

    _Spacetime() {}
    _Spacetime(const Number &x, const Number &y, const Number &z, const Time &_t)
        : space(x, y, z),
          time(_t) {}
    _Spacetime(const Space &s, const Time &t)
        : space(s),
          time(t) {}

    _Spacetime &operator=(const Space &s) {
        space = s;
        return *this;
    }
    _Spacetime &operator=(const Time &t) {
        time = t;
        return *this;
    }

    _Spacetime &operator+(const Space &s) {
        space += s;
        return *this;
    }
    _Spacetime &operator+(const Time &t) {
        time = time + t;
        return *this;
    }

    operator Space() const { return const_cast<const Space &>(space); }
    operator Time() const { return const_cast<const Time &>(time); }

    friend OStream &operator<<(OStream &os, const _Spacetime &st) {
        os << "{" << st.space << ",t=" << st.time << "}";
        return os;
    }

    Space space;
    Time time;
} __attribute__((packed));

template <Scale S>
class _Region : public Sphere<typename Select_Scale<S>::Number, typename Select_Scale<S>::Unsigned_Number>,
                public Time_Interval // no need for padding: 3x8+8 | 3x16+16 | 3x32+32
{
  public:
    using _Sphere = Sphere<typename Select_Scale<S>::Number, typename Select_Scale<S>::Unsigned_Number>;
    using typename _Sphere::Center;
    using typename _Sphere::Number;
    using typename _Sphere::Radius;

    _Region() {}
    _Region(const Number &x, const Number &y, const Number &z, const Radius &r, const Time &t0, const Time &t1)
        : _Sphere(Center(x, y, z), r),
          Time_Interval(t0, t1) {}
    _Region(const Center &c, const Radius &r, const Time &t0, const Time &t1)
        : _Sphere(c, r),
          Time_Interval(t0, t1) {}
    _Region(const _Spacetime<S> &st, const Radius &r, const Time &t1)
        : _Sphere(_Space<S>(st), r),
          Time_Interval(st, t1) {}

    operator _Spacetime<S>() const { return _Spacetime<S>(this->center(), t0); }

    bool operator==(const _Region &r) const { return !memcmp(this, &r, sizeof(_Region)); }
    bool operator!=(const _Region &r) const { return !(*this == r); }

    bool contains(const _Spacetime<S> &st) const {
        return Time_Interval::contains(st.time) && _Sphere::contains(st.space);
    }
    bool contains(const Center &c, const Time &t) const { return Time_Interval::contains(t) && _Sphere::contains(c); }

    friend OStream &operator<<(OStream &os, const _Region &r) {
        os << "{" << reinterpret_cast<const _Sphere &>(r) << ",t0=" << r.t0 << ",t1=" << r.t1 << "}";
        return os;
    }
} __attribute__((packed));

template <>
class _Region<_NULL> : public Time_Interval // no need for padding: 3x8+8 | 3x16+16 | 3x32+32
{
  public:
    using Number = typename Select_Scale<_NULL>::Number;
    using Radius = typename Select_Scale<_NULL>::Unsigned_Number;
    using Center = Point<Select_Scale<_NULL>::Number, 3>;

    _Region()
        : Time_Interval(0, 0) {}
    _Region(const Number &x, const Number &y, const Number &z, const Radius &r, const Time &t0, const Time &t1)
        : Time_Interval(t0, t1) {}
    _Region(const Center &c, const Radius &r, const Time &t0, const Time &t1)
        : Time_Interval(t0, t1) {}
    _Region(const _Spacetime<_NULL> &st, const Radius &r, const Time &t1)
        : Time_Interval(st, t1) {}

    operator _Spacetime<_NULL>() const { return _Spacetime<_NULL>(Center(0, 0, 0), t0); }

    bool operator==(const _Region &r) const { return r.t0 == t0 && r.t1 == t1; }
    bool operator!=(const _Region &r) const { return !(*this == r); }

    bool contains(const _Spacetime<_NULL> &st) const { return Time_Interval::contains(st.time); }
    bool contains(const Center &c, const Time &t) const { return Time_Interval::contains(t); }

    Radius radius() const { return 0; }
    Center center() const { return Center(0, 0, 0); }

    friend OStream &operator<<(OStream &os, const _Region &r) {
        os << "{t0=" << r.t0 << ",t1=" << r.t1 << "}";
        return os;
    }
} __attribute__((packed));
}; // namespace Space_Time

// SmartData basic definitions used also by the associated communication protocols
class SmartData {
  protected:
    static const UInt32 PAN              = 10;    // Nodes
    static const UInt32 LAN              = 100;   // Nodes
    static const UInt32 WAN              = 10000; // Nodes
    static const UInt32 NODES            = Traits<Build>::NODES;
    static const Space_Time::Scale SCALE = Traits<Network>::routing
                                               ? ((Traits<Build>::NODES <= PAN)   ? Space_Time::CMx50_8
                                                  : (Traits<Build>::NODES <= WAN) ? Space_Time::CM_16
                                                                                  : Space_Time::CM_32)
                                               : Space_Time::_NULL;

  public:
    using Time          = Space_Time::Time;
    using Short_Time    = Space_Time::Short_Time;
    using Time_Interval = Space_Time::Time_Interval;
    using Time_Offset   = Space_Time::Time_Offset;
    using Scale         = Space_Time::Scale;

    // temporary --> to be removed when multi unit is merged
    typedef struct __attribute__((packed)) _Motion_Vector {

        // size -- total
        bool _valid;        // 1    -- 1 skip first byte as it is used just for control of shared memory communication
        Int32 _location[3]; // 4*3  -- 13
        Float32 _speed;     // 4    -- 17
        Float32 _heading;   // 4    -- 21
        Float32 _yaw_rate;  // 4    -- 25
        Float32 _acceleration; // 4    -- 29
        UInt64 _id;            // 8    -- 37
        Int32 _uncertainty;    // 4    -- 41
        Int32 _obj_class;      // 4    -- 45
        UInt64 _timestamp;     // 8    -- 53

        void log() {
            db<SmartData>(LOGGER) << "lon=" << _location[0] << ",\n\t";
            db<SmartData>(LOGGER) << "lat=" << _location[1] << ",\n\t";
            db<SmartData>(LOGGER) << "alt=" << _location[2] << ",\n\t";
            db<SmartData>(LOGGER) << "speed=" << _speed << ",\n\t";
            db<SmartData>(LOGGER) << "heading=" << _heading << ",\n\t";
            db<SmartData>(LOGGER) << "yawr=" << _yaw_rate << ",\n\t";
            db<SmartData>(LOGGER) << "accel=" << _acceleration << ",\n\t";
            db<SmartData>(LOGGER) << "id=" << _id << ",\n\t";
            db<SmartData>(LOGGER) << "uncertainty=" << _uncertainty << ",\n\t";
            db<SmartData>(LOGGER) << "class=" << _obj_class;
        }

    } Motion_Vector;

    typedef struct _Wheel_Telemetry {
        // size -- total
        Float32 _wheel_speed;         // 4    -- 4
        Float32 _wheel_lat_force;     // 4    -- 8
        Float32 _wheel_long_force;    // 4    -- 12
        Float32 _wheel_tire_load;     // 4    -- 16
        Float32 _wheel_torque;        // 4    -- 20
        Float32 _wheel_lat_slip;      // 4    -- 24
        Float32 _wheel_long_slip;     // 4    -- 28
        Float32 _wheel_tire_friction; // 4    -- 32
    } Wheel_Telemetry_Value;

    template <UInt32 LEN> struct Digital {
        Digital() {}
        Digital(UInt32 v) { memset(_data, v, LEN); }
        Digital(const Digital<LEN> &other) {
            for (UInt32 i = 0; i < LEN; i++)
                _data[i] = other[i];
        }

        template <typename T> void operator+=(const T data) {
            for (UInt32 i = 0; i < ((sizeof(T) > LEN) ? LEN : sizeof(T)); i++)
                _data[i] += data[i];
        }

        unsigned char &operator[](const UInt32 i) { return _data[i]; }
        const unsigned char &operator[](const UInt32 i) const { return _data[i]; }
        operator const unsigned char *() const { return _data; }
        operator unsigned char *() { return _data; }

        unsigned char _data[LEN];
    };

    // SI Unit defining the SmartData semantics (inspired by IEEE 1451 TEDs)
    class Unit {
      public:
        // Formats
        // Bit       31     30     28                   16                                     0
        //         +--+------+------+--------------------+-------------------------------------+
        // Digital |0 | FRAG |  MSD | type               | len                                 |
        //         +--+------+------+--------------------+-------------------------------------+

        // Bit       31   29   27     24     21     18     15     12      9      6      3      0
        //         +--+----+----+------+------+------+------+------+------+------+------+------+
        // SI      |1 |NUM |MOD |sr+4  |rad+4 |m+4   |kg+4  |s+4   |A+4   |K+4   |mol+4 |cd+4  |
        //         +--+----+----+------+------+------+------+------+------+------+------+------+
        // Bits     1   2    2     3      3      3      3      3      3      3      3      3

        // Valid values for field SI
        enum : UInt32 {
            DIGITAL = 0U << 31, // The Unit is plain digital data. Subsequent 15 bits designate the data type. Lower 16
                                // bits are payloadlication-specific, usually a device selector.
            SI  = 1U << 31,     // The Unit is SI. Remaining bits are interpreted as specified here.
            SID = SI
        };

        // Valid values for field NUM
        enum : UInt32 {
            I32 = 0 << 29, // Value is an integral int stored in the 32 last significant bits of a 32-bit big-endian
                           // integer.
            I64 = 1 << 29, // Value is an integral int stored in the 64 last significant bits of a 64-bit big-endian
                           // integer.
            F32 = 2 << 29, // Value is a real int stored as an IEEE 754 binary32 big-endian floating point.
            D64 =
                3
                << 29, // Value is a real int stored as an IEEE 754 binary64 big-endian double precision floating point.
            NUM = D64  // AND mask to select NUM bits
        };

        // Valid values for field MOD
        enum : UInt32 {
            DIR = 0 << 27,     // Unit is described by the product of SI base units raised to the powers recorded in the
                               // remaining fields.
            DIV = 1 << 27,     // Unit is U/U, where U is described by the product SI base units raised to the powers
                               // recorded in the remaining fields.
            LOG = 2 << 27,     // Unit is log_e(U), where U is described by the product of SI base units raised to the
                               // powers recorded in the remaining fields.
            LOG_DIV = 3 << 27, // Unit is log_e(U/U), where U is described by the product of SI base units raised to the
                               // powers recorded in the remaining fields.
            MOD = LOG_DIV      // AND mask to select MOD bits
        };

        // Masks to select the SI units
        enum : UInt32 {
            SR  = 7 << 24,
            RAD = 7 << 21,
            M   = 7 << 18,
            KG  = 7 << 15,
            S   = 7 << 12,
            A   = 7 << 9,
            K   = 7 << 6,
            MOL = 7 << 3,
            CD  = 7 << 0
        };

        // Mask to select field LEN of digital data
        enum : UInt32 { LEN = (1 << 16) - 1 };

        // Valid values for field MSD of digital data
        enum : UInt32 {
            NOT_MSD    = 0 << 29,       // Regular Digital Data
            MULTI_UNIT = 1 << 29,       // Value is an integral int stored in the 32 last significant bits of a 32-bit
                                        // big-endian integer.
            MULTI_VALUE = 2 << 29,      // Value is an integral int stored in the 64 last significant bits of a 64-bit
                                        // big-endian integer.
            MULTI_DEVICE = 3 << 29,     // Value is a real int stored as an IEEE 754 binary32 big-endian floating point.
            MSD          = MULTI_DEVICE // AND mask to select MSD bits
        };

        // Helper to create digital units
        template <UInt32 _MSD, UInt32 _TYPE, UInt32 _SUBTYPE, UInt32 _LEN> class Digital_Unit {
          public:
            enum : UInt32 { UNIT = DIGITAL | _MSD << 29 | _TYPE << 24 | _SUBTYPE << 16 | _LEN << 0 };

          private:
            // Compile-time verifications
            static const typename IF<(_MSD & (~((1 << 3) - 1))), void, bool>::Result Invalid_MSD          = false;
            static const typename IF<(_TYPE & (~((1 << 23) - 1))), void, bool>::Result Invalid_TYPE       = false;
            static const typename IF<(_SUBTYPE & (~((1 << 15) - 1))), void, bool>::Result Invalid_SUBTYPE = false;
            static const typename IF<(_LEN & (~LEN)), void, bool>::Result Invalid_LEN                     = false;
        };

        // Helper to create SI units
        template <int _MOD, int _SR, int _RAD, int _M, int _KG, int _S, int _A, int _K, int _MOL, int _CD>
        class SI_Unit {
          public:
            //                     SI  |  MOD  |        sr       |         rad      |        m       |        kg       |
            //                     s       |        A      |        K      |       mol       |     cd
            enum : UInt32 {
                UNIT = SI | _MOD | (4 + _SR) << 24 | (4 + _RAD) << 21 | (4 + _M) << 18 | (4 + _KG) << 15 |
                       (4 + _S) << 12 | (4 + _A) << 9 | (4 + _K) << 6 | (4 + _MOL) << 3 | (4 + _CD)
            };

          private:
            // Compile-time verifications
            static const typename IF<(_MOD & (~MOD)), void, bool>::Result Invalid_MOD      = false;
            static const typename IF<((_SR + 4) & (~7u)), void, bool>::Result Invalid_SR   = false;
            static const typename IF<((_RAD + 4) & (~7u)), void, bool>::Result Invalid_RAD = false;
            static const typename IF<((_M + 4) & (~7u)), void, bool>::Result Invalid_M     = false;
            static const typename IF<((_KG + 4) & (~7u)), void, bool>::Result Invalid_KG   = false;
            static const typename IF<((_S + 4) & (~7u)), void, bool>::Result Invalid_S     = false;
            static const typename IF<((_A + 4) & (~7u)), void, bool>::Result Invalid_A     = false;
            static const typename IF<((_K + 4) & (~7u)), void, bool>::Result Invalid_K     = false;
            static const typename IF<((_MOL + 4) & (~7u)), void, bool>::Result Invalid_MOL = false;
            static const typename IF<((_CD + 4) & (~7u)), void, bool>::Result Invalid_CD   = false;
        };

        // Typical SI Quantities
        enum Quantity : UInt32 {
            //                                mod,     sr,    rad,      m,     kg,      s,      A,      K,    mol, cd
            //                                unit
            Acceleration        = SI_Unit<DIR, +0, +0, +1, +0, -2, +0, +0, +0, +0>::UNIT, // m/s2
            Angle               = SI_Unit<DIR, +0, +1, +0, +0, +0, +0, +0, +0, +0>::UNIT, // rad
            Amount_of_Substance = SI_Unit<DIR, +0, +0, +0, +0, +0, +0, +0, +1, +0>::UNIT, // mol
            Angular_Velocity    = SI_Unit<DIR, +0, +1, +0, +0, -1, +0, +0, +0, +0>::UNIT, // rad/s
            Area                = SI_Unit<DIR, +0, +0, +2, +0, +0, +0, +0, +0, +0>::UNIT, // m2
            Current             = SI_Unit<DIR, +0, +0, +0, +0, +0, +1, +0, +0, +0>::UNIT, // Ampere
            Electric_Current    = Current,
            Force               = SI_Unit<DIR, +0, +0, +1, +1, -2, +0, +0, +0, +0>::UNIT, // Newton
            Humidity            = SI_Unit<DIR, +0, +0, -3, +1, +0, +0, +0, +0, +0>::UNIT, // kg/m3
            Length              = SI_Unit<DIR, +0, +0, +1, +0, +0, +0, +0, +0, +0>::UNIT, // m
            Luminous_Intensity  = SI_Unit<DIR, +0, +0, +0, +0, +0, +0, +0, +0, +1>::UNIT, // cd
            Mass                = SI_Unit<DIR, +0, +0, +0, +1, +0, +0, +0, +0, +0>::UNIT, // kg
            Power               = SI_Unit<DIR, +0, +0, +2, +1, -3, +0, +0, +0, +0>::UNIT, // Watt
            Pressure            = SI_Unit<DIR, +0, +0, -1, +1, -2, +0, +0, +0, +0>::UNIT, // Pascal
            Velocity            = SI_Unit<DIR, +0, +0, +1, +0, -1, +0, +0, +0, +0>::UNIT, // m/s
            Sound_Intensity     = SI_Unit<DIR, +0, +0, +0, +1, -3, +0, +0, +0, +0>::UNIT, // W/m2
            Temperature         = SI_Unit<DIR, +0, +0, +0, +0, +0, +0, +1, +0, +0>::UNIT, // Kelvin
            Torque              = SI_Unit<DIR, +0, +0, +2, +1, -2, +0, +0, +0, +0>::UNIT, // Nm
            Time                = SI_Unit<DIR, +0, +0, +0, +0, +1, +0, +0, +0, +0>::UNIT, // s
            Speed               = Velocity,
            Volume              = SI_Unit<DIR, +0, +0, +3, +0, +0, +0, +0, +0, +0>::UNIT, // m3
            Voltage             = SI_Unit<DIR, +0, +0, +2, +1, -3, -1, +0, +0, +0>::UNIT, // Volt
            Water_Flow          = SI_Unit<DIR, +0, +0, +3, +0, -1, +0, +0, +0, +0>::UNIT, // m3/s
            Mass_Flow           = SI_Unit<DIR, +0, +0, +0, +1, -1, +0, +0, +0, +0>::UNIT, // kg/s
            Frequency           = SI_Unit<DIR, +0, +0, +0, +0, -1, +0, +0, +0, +0>::UNIT, // Hz

            Stiffness = SI_Unit<DIR, +0, +0, +0, +1, -2, +0, +0, +0, +0>::UNIT, // N/m
            Damping   = SI_Unit<DIR, +0, +0, +0, +1, -1, +0, +0, +0, +0>::UNIT, // N s/m

            Ratio   = SI_Unit<LOG_DIV, -4, -4, -4, -4, -4, -4, -4, -4, -4>::UNIT, // not an SI unit
            Percent = SI_Unit<LOG_DIV, -4, -4, -4, -4, -4, -4, -4, -4, -3>::UNIT, // not an SI unit, a ratio < 1
            PPM = SI_Unit<LOG_DIV, -4, -4, -4, -4, -4, -4, -4, -4, -2>::UNIT, // not an SI unit, a ratio in parts per
                                                                              // million
            PPB = SI_Unit<LOG_DIV, -4, -4, -4, -4, -4, -4, -4, -4, -1>::UNIT, // not an SI unit, a ratio in parts per
                                                                              // billion
            Relative_Humidity = SI_Unit<LOG_DIV, -4, -4, -4, -4, -4, -4, -4, -4, +0>::
                UNIT, // not an SI unit, a percentage representing the partial pressure of water vapor in the mixture to
                      // the equilibrium vapor pressure of water over a flat surface of pure water at a given
                      // temperature
            Power_Factor = SI_Unit<LOG_DIV, -4, -4, -4, -4, -4, -4, -4, -4, +1>::
                UNIT, // not an SI unit, a ratio of the real power absorbed by the load to the payloadarent power flowing in
                      // the circuit; a dimensionless number in [-1,1]
            Counter = SI_Unit<LOG_DIV, -4, -4, -4, -4, -4, -4, -4, -4, +2>::UNIT, // not an SI unit, the current value
                                                                                  // of an external counter
            Antigravity = SI_Unit<LOG_DIV, +3, +3, +3, +3, +3, +3, +3, +3, +3>::UNIT, // for Dummy_Transducer :-)
            Suspension_Prediction = SI_Unit<LOG_DIV, +0, +0, +0, +0, +0, +0, +0, +0, +0>::UNIT,
            Road_Condition_Class  = SI_Unit<DIR, -4, -4, -4, -4, -4, -4, -4, -3, +0>::UNIT, // not an SI unit, an enum
                                                                                            // for road condition class.
        };

        // Digital data types
        enum Digital_Data : UInt32 {
            //                                    msd, type, subtype,   length
            // Switches
            Direction = Digital_Unit<0, 0, 1, 1>::UNIT,
            Switch    = Digital_Unit<0, 0, 0, 1>::UNIT,
            On_Off    = Switch,

            // RFIDs and SmartCartds
            RFID32 = Digital_Unit<0, 1, 0, 4>::UNIT,

            // Audio and Video (from RTP)                                       A/V         Hz        Ch       Ref
            PCMU    = Digital_Unit<0, 2, 0, 0>::UNIT,  // A         8000        1       [RFC3551]
            GSM     = Digital_Unit<0, 2, 3, 0>::UNIT,  // A         8000        1       [RFC3551]
            G723    = Digital_Unit<0, 2, 4, 0>::UNIT,  // A         8000        1       [Vineet_Kumar][RFC3551]
            DVI4_8  = Digital_Unit<0, 2, 5, 0>::UNIT,  // A         8000        1       [RFC3551]
            DVI4_16 = Digital_Unit<0, 2, 6, 0>::UNIT,  // A        16000        1       [RFC3551]
            LPC     = Digital_Unit<0, 2, 7, 0>::UNIT,  // A         8000        1       [RFC3551]
            PCMA    = Digital_Unit<0, 2, 8, 0>::UNIT,  // A         8000        1       [RFC3551]
            G722    = Digital_Unit<0, 2, 9, 0>::UNIT,  // A         8000        1       [RFC3551]
            L16_2   = Digital_Unit<0, 2, 10, 0>::UNIT, // A        44100        2       [RFC3551]
            L16_1   = Digital_Unit<0, 2, 11, 0>::UNIT, // A        44100        1       [RFC3551]
            QCELP   = Digital_Unit<0, 2, 12, 0>::UNIT, // A         8000        1       [RFC3551]
            CN      = Digital_Unit<0, 2, 13, 0>::UNIT, // A         8000        1       [RFC3389]
            MPA     = Digital_Unit<0, 2, 14, 0>::UNIT, // A        90000                [RFC3551][RFC2250]
            G728    = Digital_Unit<0, 2, 15, 0>::UNIT, // A         8000        1       [RFC3551]
            DVI4_11 = Digital_Unit<0, 2, 16, 0>::UNIT, // A        11025        1       [Joseph_Di_Pol]
            DVI4_22 = Digital_Unit<0, 2, 17, 0>::UNIT, // A        22050        1       [Joseph_Di_Pol]
            G729    = Digital_Unit<0, 2, 18, 0>::UNIT, // A         8000        1       [RFC3551]
            CelB    = Digital_Unit<0, 2, 25, 0>::UNIT, // V        90000                [RFC2029]
            JPEG    = Digital_Unit<0, 2, 26, 0>::UNIT, // V        90000                [RFC2435]
            nv      = Digital_Unit<0, 2, 28, 0>::UNIT, // V        90000                [RFC3551]
            H261    = Digital_Unit<0, 2, 31, 0>::UNIT, // V        90000                [RFC4587]
            MPV     = Digital_Unit<0, 2, 32, 0>::UNIT, // V        90000                [RFC2250]
            MP2T    = Digital_Unit<0, 2, 33, 0>::UNIT, // AV       90000                [RFC2250]
            H263    = Digital_Unit<0, 2, 34, 0>::UNIT, // V        90000                [Chunrong_Zhu]
            MPEG2TS = Digital_Unit<0, 2, 35, 0>::UNIT, // V        90000                [Chunrong_Zhu]
            PNG     = Digital_Unit<0, 2, 36, 0>::UNIT,
            RAW_BGR = Digital_Unit<0, 2, 37, 0>::UNIT, // blue, red, green

            PCD_MONOCROMATIC = Digital_Unit<0, 3, 1, 0>::UNIT, // x, y, z, c
            PCD_RGB          = Digital_Unit<0, 3, 2, 0>::UNIT, // x, y, z, r, g, b

            // Multi SmartData
            MOTION_VECTOR_GLOBAL = Digital_Unit<1, 0, 0, 0>::UNIT,
            MOTION_VECTOR_LOCAL  = Digital_Unit<1, 1, 0, 0>::UNIT,

            // Road Surface Condition
            ROAD_SURFACE_CONDITION = Digital_Unit<2, 0, 0, 4>::UNIT,

            // Telemetry
            WHEEL_TELEMETRY = Digital_Unit<2, 4, 0, 32>::UNIT, // 8x4 bytes

            // SEU related units do not use as SmartData
            MONITOR      = Digital_Unit<0, 31, 0, 1>::UNIT,
            SAFETY_MODEL = Digital_Unit<0, 31, 1, 1>::UNIT,

            // experimenting -- Deprecated (do not use)
            CARLA_IMAGE = JPEG,
            GPS3I       = Digital_Unit<1, 3, 0, 3 * 4>::UNIT,
            Dynamics    = Digital_Unit<1, 3, 1, 11 * 4 + 1>::UNIT, // 1 vehicle dynamics (x, y, z, yaw, speed, steer,
                                                                   // is-vehicle, bounding box(x,y,z),id)
            Dynamics_Array =
                Digital_Unit<1, 3, 2, 1 + (11 * 4 + 1) * 150>::UNIT, // 1 char for counter of vehicles + up to 150
                                                                     // vehicle dynamics (x, y, z, yaw, speed, steer,
                                                                     // is-vehicle, bounding box(x,y,z),id)
            ROUTE_POINTS = Digital_Unit<1, 3, 2, 3 * 4 * 100>::UNIT
        };

        // SI Factors
        typedef char Factor;
        enum {
            // Name           Code         Symbol    Factor
            ATTO  = (8 - 8), //     a       0.000000000000000001
            FEMTO = (8 - 7), //     f       0.000000000000001
            PICO  = (8 - 6), //     p       0.000000000001
            NANO  = (8 - 5), //     n       0.000000001
            MICRO = (8 - 4), //     μ       0.000001
            MILI  = (8 - 3), //     m       0.001
            CENTI = (8 - 2), //     c       0.01
            DECI  = (8 - 1), //     d       0.1
            NONE  = (8),     //     -       1
            DECA  = (8 + 1), //     da      10
            HECTO = (8 + 2), //     h       100
            KILO  = (8 + 3), //     k       1000
            MEGA  = (8 + 4), //     M       1000000
            GIGA  = (8 + 5), //     G       1000000000
            TERA  = (8 + 6), //     T       1000000000000
            PETA  = (8 + 7)  //     P       1000000000000000
        };

        template <UInt32 UNIT> struct Get {
            static const size_t UNKNOWN = 1;
            static const size_t DIGITAL_LEN =
                (UNIT & SID) == SI                             ? 0
                : (UNIT >> 24) == (MOTION_VECTOR_GLOBAL >> 24) ? ((UNIT & LEN) * sizeof(Motion_Vector)) + 1
                : (UNIT >> 24) == (MOTION_VECTOR_LOCAL >> 24)
                    ? ((UNIT & LEN) * sizeof(Motion_Vector)) + 1
                    :
                    // To be filled when used: UNIT & LEN can yield a config, therefore, extend the if to include all
                    // configurations as the one below:
                    (UNIT >> 16) == (PCMU >> 16)  ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (GSM >> 16)     ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (G723 >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (DVI4_8 >> 16)  ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (DVI4_16 >> 16) ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (LPC >> 16)     ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (PCMA >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (G722 >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (L16_2 >> 16)   ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (L16_1 >> 16)   ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (QCELP >> 16)   ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (CN >> 16)      ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (MPA >> 16)     ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (G728 >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (DVI4_11 >> 16) ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (DVI4_22 >> 16) ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (G729 >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (CelB >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (JPEG >> 16)    ? ((UNIT & LEN) == 1 ? 61440 : UNIT & LEN)
                : (UNIT >> 16) == (nv >> 16)      ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (H261 >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (MPV >> 16)     ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (MP2T >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (H263 >> 16)    ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (MPEG2TS >> 16) ? ((UNIT & LEN) == 1 ? UNKNOWN : UNIT & LEN)
                : (UNIT >> 16) == (PNG >> 16)
                    ? ((UNIT & LEN) == 1 ? 1 * 1024 * 1024 : ((UNIT & LEN) == 2 ? 4 * 1024 * 1024 : UNIT & LEN))
                : (UNIT >> 16) == (RAW_BGR >> 16)
                    ? ((UNIT & LEN) == 1 ? 1920 * 1080 * 3 : ((UNIT & LEN) == 2 ? 1920 * 1200 * 3 : UNIT & LEN))
                    :
                    // (x, y, z, i) * sizeof(float) * channels * points_per_second*delta_t (e.g., every 0.1 seconds) #
                    // maximum size, once lidars dropoff intensity 0
                    (UNIT >> 16) == (PCD_MONOCROMATIC >> 16)
                    ? ((UNIT & LEN) == 1 ? 4 * 4 * 32 * (100000 * 0.1)
                                         : ((UNIT & LEN) == 2 ? 16 * 2 * 12 * (4 * 3 + 1) : UNIT & LEN))
                : (UNIT >> 16) == (PCD_RGB >> 16) ? ((UNIT & LEN) == 1 ? 1 * 1024 * 1024 : UNIT & LEN)
                                                  : UNIT & LEN;

            typedef typename IF<
                ((UNIT & SID) == SI) && ((UNIT & NUM) == I32),
                Int32,
                typename IF<
                    ((UNIT & SID) == SI) && ((UNIT & NUM) == I64),
                    Int64,
                    typename IF<
                        ((UNIT & SID) == SI) && ((UNIT & NUM) == F32),
                        Float32,
                        typename IF<((UNIT & SID) == SI) && ((UNIT & NUM) == D64),
                                    Float64,
                                    typename IF<((UNIT & SID) == DIGITAL) && (DIGITAL_LEN == 1),
                                                Int8,
                                                typename IF<((UNIT & SID) == DIGITAL) && (DIGITAL_LEN == 2),
                                                            Int16,
                                                            typename IF<((UNIT & SID) == DIGITAL) && (DIGITAL_LEN == 4),
                                                                        Int32,
                                                                        typename IF<((UNIT & SID) == DIGITAL),
                                                                                    struct Digital<DIGITAL_LEN>,
                                                                                    void>::Result>::Result>::Result>::
                                        Result>::Result>::Result>::Result>::Result Type;
        };

        template <typename T> struct GET;

        template <UInt32 U> struct Wrap {
            enum : UInt32 { UNIT = U };
        };

      public:
        Unit()
            : _unit(0) {}
        Unit(UInt32 u) { _unit = u; }

        operator UInt32() const { return _unit; }

        bool operator==(const Unit &u) const {
            if (!(_unit & SI) &&
                (((_unit >> 24) == (MOTION_VECTOR_LOCAL >> 24)) || ((_unit >> 24) == (MOTION_VECTOR_GLOBAL >> 24))))
                return (_unit >> 24) == (u >> 24); // ignore subtype (class of motion vector local or global)
            return _unit == u._unit;
        }

        size_t value_size() const {
            return (_unit & SI) && ((_unit & NUM) == I32)   ? sizeof(Int32)
                   : (_unit & SI) && ((_unit & NUM) == I64) ? sizeof(Int64)
                   : (_unit & SI) && ((_unit & NUM) == F32) ? sizeof(Float32)
                   : (_unit & SI) && ((_unit & NUM) == D64) ? sizeof(Float64)
                   : !(_unit & SI)                          ? digital_value_size(_unit)
                                                            : 0;
        }

        static size_t digital_value_size(UInt32 unit) {
            if (unit & SI) return 0;

            if ((unit >> 24) == (MOTION_VECTOR_GLOBAL >> 24))
                // # elements * (1 byte to tell if ended + 3*(I32 Loc) + F32 speed + F32 Heading + F32 YawR. + F32
                // Accel) + 1 to tell if next is valid plus ID and Confidence and class + I64 Time (to be removed after
                // modifications in UNIT and MultiSmartData)
                return ((unit & LEN) * (1 + (3 * 4 + 4 * 4) + 8 + 4 + 4 + 8)) + 1;

            if ((unit >> 24) == (MOTION_VECTOR_LOCAL >> 24))
                // # elements * (1 byte to tell if ended + 3*(F32 Loc) + F32 speed + F32 Heading + F32 YawR. + F32
                // Accel) + 1 to tell if next is valid plus ID and Confidence and class + I64 Time (to be removed after
                // modifications in UNIT and MultiSmartData)
                return ((unit & LEN) * (1 + (3 * 4 + 4 * 4) + 8 + 4 + 4 + 8)) + 1;

            switch (unit >> 16) {
            case JPEG >> 16:
                return (unit & LEN) == 1 ? 61440 : unit & LEN;
            case PCMU >> 16: // TODO
            case GSM >> 16:
            case G723 >> 16:
            case DVI4_8 >> 16:
            case DVI4_16 >> 16:
            case LPC >> 16:
            case PCMA >> 16:
            case G722 >> 16:
            case L16_2 >> 16:
            case L16_1 >> 16:
            case QCELP >> 16:
            case CN >> 16:
            case MPA >> 16:
            case G728 >> 16:
            case DVI4_11 >> 16:
            case DVI4_22 >> 16:
            case G729 >> 16:
            case CelB >> 16:
            case nv >> 16:
            case H261 >> 16:
            case MPV >> 16:
            case MP2T >> 16:
            case H263 >> 16:
            case MPEG2TS >> 16:
                return ((unit & LEN) == 1 ? 1 : unit & LEN);
            case PNG >> 16:
                return ((unit & LEN) == 1 ? 1 * 1024 * 1024 : ((unit & LEN) == 2 ? 4 * 1024 * 1024 : unit & LEN));
            case RAW_BGR >> 16:
                return ((unit & LEN) == 1 ? 1920 * 1080 * 3 : ((unit & LEN) == 2 ? 1920 * 1200 * 3 : unit & LEN));
            case PCD_MONOCROMATIC >> 16:
                return ((unit & LEN) == 1
                            ? 4 * 4 * 32 * (100000 * 0.1)
                            : ((unit & LEN) == 2
                                   ? 16 * 2 * 12 * (4 * 3 + 1)
                                   : unit & LEN)); // Config 2: VLP16_POINTS_PER_PACKET = VLP16_POINTS_PER_BLOCK (16*2)
                                                   // * VLP16_BLOCKS_PER_PACKET (12) * LIDAR_Point (3 floats for x,y,z
                                                   // distance and 1 for char for intensity)
            case PCD_RGB >> 16:
                return ((unit & LEN) == 1 ? 1 * 1024 * 1024 : unit & LEN);
            default:
                return unit & LEN;
            }
        }

        int sr() const { return ((_unit & SR) >> 24) - 4; }
        int rad() const { return ((_unit & RAD) >> 21) - 4; }
        int m() const { return ((_unit & M) >> 18) - 4; }
        int kg() const { return ((_unit & KG) >> 15) - 4; }
        int s() const { return ((_unit & S) >> 12) - 4; }
        int a() const { return ((_unit & A) >> 9) - 4; }
        int k() const { return ((_unit & K) >> 6) - 4; }
        int mol() const { return ((_unit & MOL) >> 3) - 4; }
        int cd() const { return ((_unit & CD) >> 0) - 4; }

        friend OStream &operator<<(OStream &os, const Unit &u) {
            if (u & SI) {
                os << "{SI";
                switch (u & MOD) {
                case DIR:
                    break;
                case DIV:
                    os << "[U/U]";
                    break;
                case LOG:
                    os << "[log(U)]";
                    break;
                case LOG_DIV:
                    os << "[log(U/U)]";
                };
                switch (u & NUM) {
                case I32:
                    os << ":I32";
                    break;
                case I64:
                    os << ":I64";
                    break;
                case F32:
                    os << ":F32";
                    break;
                case D64:
                    os << ":D64";
                }
                os << ':';
                if (u.sr()) os << "sr^" << u.sr() << '.';
                if (u.rad()) os << "rad^" << u.rad() << '.';
                if (u.m()) os << "m^" << u.m() << '.';
                if (u.kg()) os << "kg^" << u.kg() << '.';
                if (u.s()) os << "s^" << u.s() << '.';
                if (u.a()) os << "A^" << u.a() << '.';
                if (u.k()) os << "K^" << u.k() << '.';
                if (u.mol()) os << "mol^" << u.mol() << '.';
                if (u.cd()) os << "cdr^" << u.cd() << '.';
                os << '\b';
            } else
                os << "{D:" << "M=" << ((u >> 29) & (0b11)) << ",t=" << ((u >> 24) & 0b11111)
                   << ",subt=" << ((u >> 16) & (0xFF)) << ",l=" << (u & LEN);
            os << "}";
            return os;
        }

      private:
        UInt32 _unit;
    } __attribute__((packed));

    // Numeric value (either integer32, integer64, float32, double64 according to Unit::NUM)
    template <UInt32 UNIT> class Value {
        typedef typename Unit::Get<UNIT>::Type Type;

      public:
        Value() {}
        Value(const Type &v)
            : _value(v) {}

        operator Type &() { return _value; }

      private:
        Type _value;
    } __attribute__((packed));

    // (Geographic) Space
    typedef Space_Time::_Space<SCALE> Space;                    // used locally and by the communication protocols
    typedef Space_Time::_Space<Space_Time::CM_32> Global_Space; // exposed to SmartData clients

    // Space-Time (not exactly SpaceTime, because this is not a Minkowski space)
    typedef Space_Time::_Spacetime<SCALE> Spacetime;

    // Spatial Region in a time interval
    typedef Space_Time::_Region<SCALE> Region;

    typedef UInt64 Signature;

    // Device enumerator (do differentiate two SmartData at the same (unit, x, y, z, t))
    typedef UInt32 Device_Id;
    enum : Device_Id { DEFAULT = 0, UNIQUE = DEFAULT };

    // Versions (for future extensions)
    typedef UInt8 Version;
    enum : Version { V0 = 0, STATIC = V0, V1 = 4, MOBILE = V1 };

    // Message Types
    typedef UInt8 Type;
    enum : Type { INTEREST = 0, RESPONSE = 1, COMMAND = 2, CONTROL = 3 };

    // SmartData Modes, operations, and message Subtypes
    typedef UInt8 Mode;
    enum : Mode {
        // Bit   7   6   5   4   3   2   1   0
        //     +---+---+---+---+---+---+---+---+
        //     |    subtype    |   op  |  mode |
        //     +---+---+---+---+---+---+---+---+
        MODE_MASK      = 0x3 << 0,
        OPERATION_MASK = 0x3 << 2,
        SUBTYPE_MASK   = 0xf << 4,

        // Interested modes
        SINGLE =
            0
            << 0, // only one response is desired for each interest (desired, but multiple responses are still possible)
        ALL = 1 << 0, // all possible responses (e.g. from different sensors) are desired
        // Interested operations
        ANNOUNCE = 0 << 2, // announce an Interest
        REVOKE   = 1 << 2, // revoke an Interest announcement

        // Responsive modes
        PRIVATE    = 0 << 0, // local access only
        ADVERTISED = 1 << 0, // advertised to the network (can bind to interests)
        COMMANDED  = 3 << 0, // commanded via the network (can bind to interests and be commanded)
        // Responsive operations
        ADVERTISE = 0 << 2, // advertise a Responsive
        CONCEAL   = 1 << 2, // conceal a Responsive advertisement (e.g. node shutdown)
        RESPOND   = 2 << 2, // repond to an interest

        // Response message subtypes
        // Bit   7   6   5   4   3   2   1   0
        //     +---+---+---+---+---+---+---+---+
        //     |   |   |A/P|I/C|   |   |   |   |
        //     +---+---+---+---+---+---+---+---+
        IMMEDIATE  = 0 << 4, // an immediate response containing the last sampled value by the Transducer
        CUMULATIVE = 1 << 4, // a response containing the value accumulated by the Transducer
        ACTUAL     = 0 << 5, // a response containing a value effectively produced by the Transducer
        PREDICTIVE = 1 << 5, // a response containing a predicted value

        // Control message subtypes
        // Security
        DH_REQUEST   = 1 << 4,
        DH_RESPONSE  = 2 << 4,
        AUTH_REQUEST = 3 << 4,
        AUTH_GRANTED = 4 << 4,
        ESA_RESPONSE = 5 << 4,
        // Timekeeper
        REPORT     = 6 << 4,
        KEEP_ALIVE = 7 << 4,
        EPOCH      = 8 << 4,
        // Predictor
        MODEL = 9 << 4
    };

    // The uncertainty of a SmartData
    // This is usually transducer-dependent and can express Accuracy, Precision, Resolution or a combination thereof
    typedef UInt8 Uncertainty;
    enum : Uncertainty {
        ANY     = 0, // for Interests
        UNKNOWN = 0  // for Responses
    };

    // Message Header
    template <Scale S> class _Header {
        // Bit  7 6  5 4 3 2 1 0
        //     +---+--+---+-----+----+----+----+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~
        //     ---+--- ~ ---+--- ~ ---+--- ~ ----+ |scl|tr|typ|ver  |mode|misc| lc |   o.x   |   o.y   |   o.z   |   pad
        //     |   o.t   |  lh.x   |  lh.y   |  lh.z   |   pad   |   lh.t   |
        //     +----------------+----+----+----+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~ ---+--- ~
        //     ---+--- ~ ---+--- ~ ---+--- ~ ----+
        // Bits        8          8    8    8   0/8/16/32 0/8/16/32 0/8/16/32  0/8/16/0     64     8/16/32   8/16/32
        // 8/16/32   8/16/0       64
        //
        // ver[3]:      0->static       4->mobile
        // type[2]:     0->interest     1->response     2->command      3->control
        // tr[1]:       0->no request   1->time request
        // scale[2]:    0->_NULL (0-bit) 1->8-bit,cm.50  2->16-bit,cm 3->32b,cm
        // subtype:
        // mode:
        //  Responsive: 0->private      1->advertised   2->commanded
        //  Interested: 0->one sample   1->all samples  2->revoke
        // loc con:     0-100
        // origin:      variable size Spacetime
        // last hop:    variable size Spacetime

      private:
        typedef unsigned char Config;
        typedef unsigned char Misc;

      public:
        _Header()
            : _unit(0) {} // by definition, there cannot be a unit "0" so this indicate and invalid/unused header
        _Header(const Mode &m)
            : _mode(m) {}
        _Header(const Unit &u,
                const Device_Id &d,
                const Type &t,
                const Mode &m,
                const Misc &mi     = 0,
                const Signature &s = 120)
            : _config((S & 0x03) << 6 | 0 << 5 | (t & 0x03) << 3 | (V1 & 0x07)),
              _mode(m),
              _misc(mi),
              _unit(u),
              _device(d),
              _signature(s) {}
        _Header(const Spacetime &o,
                const Unit &u,
                const Device_Id &d,
                const Type &t,
                const Mode &m,
                const Misc &mi     = 0,
                const Signature &s = 120)
            : _config((S & 0x03) << 6 | 0 << 5 | (t & 0x03) << 3 | (V1 & 0x07)),
              _mode(m),
              _misc(mi),
              _origin(o),
              _unit(u),
              _device(d),
              _signature(s) {}
        // Attributes not initialized by the constructors are filled in by the Network stack during marshaling

        Version version() const { return static_cast<Version>(_config & 0x07); }
        void version(const Version &v) { _config = (_config & 0xf8) | (v & 0x07); }

        bool mobile() const { return ((_config & 0xf8) == 4); }
        void mobile(bool m) { _config = (_config & 0xf8) | (m ? 4 : 0); }

        Type type() const { return static_cast<Type>((_config >> 3) & 0x03); }
        void type(const Type &t) { _config = (_config & 0xe7) | ((t & 0x03) << 3); }

        Mode mode() const { return _mode; }
        void mode(const Mode &m) { _mode = m; }

        Mode subtype() const { return _mode & SUBTYPE_MASK; }
        void subtype(const Mode &m) { _mode = (m & SUBTYPE_MASK) | (_mode & ~SUBTYPE_MASK); }

        Mode operation() const { return _mode & OPERATION_MASK; }
        void operation(const Mode &m) { _mode = (m & OPERATION_MASK) | (_mode & ~OPERATION_MASK); }

        Misc misc() const { return _misc; }
        void misc(const Misc &m) { _misc = m; }

        bool time_request() const { return (_config >> 5) & 0x01; }
        void time_request(bool tr) { _config = (_config & 0xdf) | (tr << 5); }

        Scale scale() const { return static_cast<Scale>((_config >> 6) & 0x03); }
        void scale(const Scale &s) { _config = (_config & 0x3f) | (s & 0x03) << 6; }

        const Percent &location_confidence() const { return _location_confidence; }
        void location_confidence(const Percent &c) { _location_confidence = c; }

        const Unit unit() const { return _unit; }
        void unit(const Unit &u) { _unit = u; }
        const Device_Id device() const { return _device; }
        void device(const Device_Id &u) { _device = u; }
        const Signature signature() const { return _signature; }
        void signature(const Signature &s) { _signature = s; }

        const Spacetime &origin() const { return _origin; }
        const Space &space() const { return reinterpret_cast<const Space &>(_origin); }
        const Time time() const { return reinterpret_cast<const Time &>(_origin); }
        void origin(const Spacetime &o) { _origin = o; }
        void origin(const Space &s) { _origin = s; }
        void origin(const Time &t) { _origin = t; }

        const Spacetime &last_hop() const { return _last_hop; }
        void last_hop(const Spacetime &lh) { _last_hop = lh; }
        void last_hop(const Space &s) { _last_hop = s; }
        void last_hop(const Time &t) { _last_hop = t; }

        size_t payload_size() const {
            size_t size;
            switch (type()) {
            case INTEREST:
                size = sizeof(Interest);
                break;
            case RESPONSE:
                size = sizeof(Response) + _unit.value_size();
                break;
            case COMMAND:
                size = sizeof(Command) + _unit.value_size();
                break;
            case CONTROL:
                size = sizeof(Control) + _unit.value_size();
                break;
            }
            return size;
        }

        friend OStream &operator<<(OStream &os, const _Header &h) {
            os << "{";
            switch (h.type()) {
            case INTEREST:
                os << "INT:";
                if (h.mode() & REVOKE)
                    os << "DEL";
                else
                    os << "ANN:" << ((h.mode() & ALL) ? "ALL" : "SGL") << ",err=" << int(h.misc());
                break;
            case RESPONSE:
                os << "RES:";
                switch (h.operation()) {
                case ADVERTISE:
                    os << "ADV:" << (((h.mode() & MODE_MASK) == COMMANDED) ? "R/W" : "R/O");
                    break;
                case CONCEAL:
                    os << "DEL";
                    break;
                case RESPOND:
                    os << "RES:" << ((h.mode() & CUMULATIVE) ? "S:" : "I:") << ((h.mode() & PREDICTIVE) ? "P" : "A");
                    break;
                default:
                    os << "ERROR!";
                    break;
                }
                break;
            case COMMAND:
                os << "CMD:   ";
                break;
            case CONTROL:
                os << "CTL:";
                switch (h.subtype()) {
                case DH_REQUEST:
                    os << "DH:REQ";
                    break;
                case DH_RESPONSE:
                    os << "DH:RSP";
                    break;
                case AUTH_REQUEST:
                    os << "AU:REQ";
                    break;
                case AUTH_GRANTED:
                    os << "AU:GRN";
                    break;
                case ESA_RESPONSE:
                    os << "ESA";
                    break;
                case REPORT:
                    os << "TM:REP";
                    break;
                case KEEP_ALIVE:
                    os << "TM:KAL";
                    break;
                case EPOCH:
                    os << "TM:EPC";
                    break;
                case MODEL:
                    os << "MODEL";
                    break;
                }
                break;
            }
            os << ",ver=" << h.version() - V0 << ",tr=" << h.time_request() << ",sc=" << h.scale()
               << ",lc=" << h._location_confidence << ",o=" << h._origin << ",u=" << h._unit << ",d=" << h._device
               << "sig=" << h._signature << ",lh=" << h._last_hop << "}";
            return os;
        }

      protected:
        Config _config;
        Mode _mode;
        Misc _misc;
        Percent _location_confidence;
        Spacetime _origin;
        Unit _unit;
        Device_Id _device;
        Signature _signature;
        Spacetime _last_hop;
    } __attribute__((packed));
    typedef _Header<SCALE> Header;

    // Interests for SmartData (used by SmartData encapsulating remote sources)
    class Interest : public Header {
      public:
        Interest(const Region &region,
                 const Unit &unit,
                 const Device_Id &device,
                 const Mode &mode,
                 const Uncertainty &uncertainty,
                 const Time &expiry,
                 const Microsecond &period)
            : Header(unit, device, INTEREST, mode, uncertainty),
              _region(region),
              _expiry(expiry),
              _period(period) {}

        Uncertainty uncertainty() const { return static_cast<Uncertainty>(_misc); }
        const Region &region() const { return _region; }
        Time expiry() const { return _expiry; }
        Microsecond period() const { return Microsecond(_period); }

        size_t data_size() const { return _unit.value_size(); };

        bool time_triggered() { return _period; }
        bool event_driven() { return !time_triggered(); }

        friend OStream &operator<<(OStream &os, const Interest &m) {
            os << "{h=" << reinterpret_cast<const Header &>(m) << ",r=" << m._region << ",x=" << m._expiry
               << ",p=" << m._period << "}";
            return os;
        }

      private:
        Region _region;
        Time _expiry;
        Short_Time _period;
    } __attribute__((packed));

    // Responses to SmartData Interests (used by SmartData encapsulating local sources, usually sensors)
    class Response : public Header {
      public:
        Response() {}
        Response(const Spacetime &origin,
                 const Unit &unit,
                 const Device_Id &device,
                 const Mode &mode,
                 const Uncertainty &uncertainty,
                 const Time &expiry,
                 const UInt16 fragment_index = 0,
                 const Signature s           = 120)
            : Header(origin, unit, device, RESPONSE, mode, uncertainty, s),
              _expiry(expiry),
              _fragment_index(fragment_index) {}

        Uncertainty uncertainty() const { return static_cast<Uncertainty>(_misc); }
        const Time &expiry() const { return _expiry; }

        template <typename T> const T &value() const { return *reinterpret_cast<const T *>(&_data); }
        template <typename T> void value(const T &v) { *reinterpret_cast<T *>(&_data) = v; }

        size_t data_size() const { return _unit.value_size(); };

        UInt16 fragment_index() const { return _fragment_index; };

        friend OStream &operator<<(OStream &os, const Response &m) {
            if (m._unit)
                os << "{h=" << reinterpret_cast<const Header &>(m) << ",x=" << m._expiry << "}";
            else
                os << "{not set}";
            return os;
        }

        template <typename T> T *data() {
            return reinterpret_cast<T *>(&_data);
        } // We need to discuss buf->frame() usage in security.cc

      private:
        Time _expiry;
        UInt32 _fragment_index; // is only != 0 when sizeof(Value) > MTU, up to 65535 fragments
        char _data[];           // must be manually allocated (adds 0 bytes to sizeof(Response); can overlap)
    } __attribute__((packed));

    // Commands to SmartData (e.g. actuation)
    class Command : public Header {
      public:
        Command(const Region &region,
                const Unit &unit,
                const Device_Id &device,
                const Mode &mode,
                const Time &expiry,
                const Microsecond &period)
            : Header(region, unit, device, COMMAND, mode),
              _radius(region.radius()),
              _t1(region.t1),
              _expiry(expiry),
              _period(period) {}

        Region region() const { return Region(_origin, _radius, _t1); }
        Time expiry() const { return _expiry; }
        Microsecond period() const { return Microsecond(_period); }

        template <typename T> const T &value() const { return *reinterpret_cast<const T *>(&_data); }
        template <typename T> void value(const T &v) { *reinterpret_cast<T *>(&_data) = v; }

        UInt32 data_size() const { return _unit.value_size(); };

        friend OStream &operator<<(OStream &os,
                                   const Command &m) { // printing m._data[0] generates a warning as _data has size 0
            os << "[CMD]:{h=" << reinterpret_cast<const Header &>(m) << ",u=" << m._unit
               << ",m=" << ((m._mode == ALL) ? 'A' : 'S') << ",x=" << m._expiry << ",re=" << m.region()
               << ",p=" << m._period << ",d=" << m._data << "}";
            return os;
        }

      private:
        Region::Radius _radius;
        Time _t1;
        Time _expiry;
        Short_Time _period;
        char _data[]; // must be manually allocated (adds 0 bytes to sizeof(Command); can overlap)
    } __attribute__((packed));

    // Control Messages
    class Control : public Header {
      protected:
        Control(const Mode &mode)
            : Header(mode) {}
        Control(const Region &region, const Unit &unit, const Device_Id &device, const Mode &mode)
            : Header(region, unit, device, CONTROL, mode),
              _radius(region.radius()),
              _t1(region.t1) {}
        Control(const Spacetime &origin, const Unit &unit, const Device_Id &device, const Mode &mode)
            : Header(origin, unit, device, CONTROL, mode),
              _radius(0),
              _t1(0) {}

        Region region() const { return Region(_origin, _radius, _t1); }

        friend OStream &operator<<(OStream &os, const Control &m) {
            os << "{h=" << reinterpret_cast<const Header &>(m) << "r=" << m.region() << "}";
            return os;
        }

      protected:
        Region::Radius _radius;
        Time _t1;
    } __attribute__((packed));

    // SmartData observer/d conditioned to Unit
    template <typename Network> using Observer = Data_Observer<typename Network::Buffer, Unit>;
    template <typename Network> using Observed = Data_Observed<typename Network::Buffer, Unit>;

    // A SmartData series as stored in a database
    struct DB_Series {
        UInt8 type;
        UInt32 unit;
        Int32 x;
        Int32 y;
        Int32 z;
        UInt32 device;
        UInt32 r;
        UInt64 t0;
        UInt64 t1;

        friend OStream &operator<<(OStream &os, const DB_Series &s) {
            os << "{t=" << s.type << ",u=" << s.unit << ",s=(" << s.x << "," << s.y << "," << s.z << "):" << s.device
               << "+" << s.r << ",t=[" << s.t0 << "," << s.t1 << "]}";
            return os;
        }
    };

    // A data-point as stored in a SmartData series database
    struct DB_Record {
        UInt8 type;
        UInt32 unit;
        Float64 value;
        UInt8 uncertainty;
        UInt8 confidence;
        Int32 x;
        Int32 y;
        Int32 z;
        UInt32 device;
        UInt64 t;

        friend OStream &operator<<(OStream &os, const DB_Record &d) {
            os << "{t=" << d.type << ",u=" << d.unit << ",d=" << d.value << ",c=" << d.confidence
               << ",e=" << d.uncertainty << ",s=(" << d.x << "," << d.y << "," << d.z << "):d=" << d.device
               << ",t=" << d.t << "}";
            return os;
        }
    } __attribute__((packed));
};

template <> struct SmartData::Unit::GET<Int32> {
    enum { NUM = I32 };
};
template <> struct SmartData::Unit::GET<Int64> {
    enum { NUM = I64 };
};
template <> struct SmartData::Unit::GET<Float32> {
    enum { NUM = F32 };
};
template <> struct SmartData::Unit::GET<Float64> {
    enum { NUM = D64 };
};

#if !defined(__smartdata_h) && !defined(__smartdata_common_only__)
#define __smartdata_h

// #include <time.h>
#include <network/tstp/tstp.h>
#include <system/thread.h>

// Local data source, possibly advertised to or commanded through the network
template <typename Transducer, typename Network>
class Responsive_SmartData : public SmartData, public Observed, private Network::Observer, public Transducer::Observer {
    friend Transducer;

  public:
    static const UInt32 UNIT             = Transducer::UNIT;
    static const Uncertainty UNCERTAINTY = Transducer::UNCERTAINTY;
    static const bool predictive         = (Traits<SmartData>::PREDICTOR != Traits<SmartData>::NONE);
    static const bool active             = Transducer::active;
    static inline const UInt32 MAX_PAYLOAD_RESPONSE =
        (Network::Packet::MTU - sizeof(Response)) / 4; // sockets only have one buffer of MTU size, so, we need to leave
                                                       // space for a read to take place before we send the next value

    typedef typename Unit::Get<UNIT>::Type Value;
    static constexpr UInt16 FRAGMENTS          = sizeof(Value) / MAX_PAYLOAD_RESPONSE;
    static constexpr UInt32 LAST_FRAGMENT_SIZE = sizeof(Value) - (FRAGMENTS * MAX_PAYLOAD_RESPONSE);

    using Space = SmartData::Global_Space;

  private:
    typedef typename Network::Buffer Buffer;
    typedef typename Network::Locator Locator;
    typedef typename Network::Timekeeper Timekeeper;
    typedef typename Select_Predictor<Traits<SmartData>::PREDICTOR>::template Predictor<Time, Value> Predictor;

    class Binding;
    typedef Simple_List<Binding> Interesteds;
    typedef Simple_List<SmartData> Responsives;

    class Binding {
      private:
        typedef typename Simple_List<Binding>::Element Element;

      public:
        Binding(const Interest &interest)
            : _region(interest.region()),
              _mode(interest.mode()),
              _uncertainty(interest.uncertainty()),
              _expiry(interest.expiry()),
              _period(interest.period()),
              _link(this) {}

        const Region &region() const { return _region; }
        const Mode &mode() const { return _mode; }
        const Uncertainty &uncertainty() const { return _uncertainty; }
        const Time &expiry() const { return _expiry; }
        const Microsecond &period() const { return _period; }

        Element *link() { return &_link; }

      private:
        Region _region;
        Mode _mode;
        Uncertainty _uncertainty;
        Time _expiry;
        Microsecond _period;

        Element _link;
    };

  public:
    Responsive_SmartData(Device_Id dev, Time expiry, Mode mode = PRIVATE, Microsecond period = 0)
        : _mode(mode),
          _origin(Locator::here(), Timekeeper::now()),
          _device(dev),
          _value(0),
          _uncertainty(UNCERTAINTY),
          _expiry(expiry),
          _signature(120),
          _transducer(new /*(SYSTEM)*/ Transducer(dev)),
          _predictor(predictive ? new /*(SYSTEM)*/ Predictor(typename Predictor::Configuration(), false) : 0),
          _thread(0),
          _link(this) {
        db<SmartData>(TRC) << "SmartData[R](d=" << dev << ",x=" << expiry << ",m="
                           << (((mode & MODE_MASK) == COMMANDED) ? "CMD" : ((mode & ADVERTISED) ? "ADV" : "PRI"))
                           << ",Fragments=" << FRAGMENTS << ",MAX_MTU_Response=" << MAX_PAYLOAD_RESPONSE
                           << ",last_frag_size=" << LAST_FRAGMENT_SIZE << ")=>" << this << endl;
        if (active)
            _transducer->attach(this);
        else if ((Transducer::TYPE & Transducer::SENSOR) && (period == 0)) {
            _value        = _transducer->sense();
            _origin       = Timekeeper::now();
            Signature sig = _transducer->signature();
            if (sig != 0) _signature = sig;
        }
        db<SmartData>(INF) << "SmartData[R]::this=" << this << "=>" << *this << endl;
        if (_mode & ADVERTISED) {
            _responsives.insert(&_link);
            Network::attach(this);
            process(ADVERTISE);
        }
        if (period > 0) {
            struct arg_struct_updater *args =
                reinterpret_cast<struct arg_struct_updater *>(malloc(sizeof(struct arg_struct_updater)));
            args->sd = this;
            _thread  = new Periodic_Thread(QUARK::Microsecond(period), &updater, static_cast<void *>(args));
            db<SmartData>(INF) << "SmartData[R]::thread=" << _thread << endl;
        }
    }

    virtual ~Responsive_SmartData() {
        db<SmartData>(TRC) << "~SmartData[R](this=" << this << ")" << endl;
        process(CONCEAL);
        Network::detach(this);
        _responsives.remove(&_link);
        if (_thread) delete _thread;
        if (_predictor) delete _predictor;
        delete _transducer;
    }

    Unit unit() const { return UNIT; }
    Mode mode() const { return _mode; }
    Uncertainty uncertainty() const { return _uncertainty; }

    Space where() const { return Locator::absolute(_origin); }
    Time when() const { return Timekeeper::absolute(_origin); }

    Time expiry() const { return _expiry; }
    bool expired() const { return Timekeeper::now() > (_origin.time + _expiry); }

    operator Value() {
        db<SmartData>(TRC) << "SmartData[R]::operator Value()[v=" << _value << "]" << endl;

        if (Transducer::TYPE & Transducer::SENSOR) {
            if (expired()) {
                if (!active) {
                    _value  = _transducer->sense();
                    _origin = Timekeeper::now();
                } else {
                    // Active transducer should have called update() timely
                    db<SmartData>(WRN) << "SmartData[R]::value(this=" << this << ",t=" << _origin.time + _expiry
                                       << ",v=" << _value << ") => expired!" << endl;
                }
            }
        } else
            db<SmartData>(WRN) << "SmartData[R]::value() called for actuation-only transducer!" << endl;

        if (_mode & CUMULATIVE) _value = 0;

        db<SmartData>(INF) << "SmartData[R]::operator Value():v=" << _value << endl;

        return _value;
    }

    SmartData &operator=(const Value &v) {
        db<SmartData>(TRC) << "SmartData[R]::operator=(v=" << v << ")" << endl;

        if (Transducer::TYPE & Transducer::ACTUATOR) {
            _transducer->actuate(v);
            db<SmartData>(TRC) << "SmartData[R]::operator=(v=" << v << ",t=" << _thread << ")" << endl;
            _value  = _transducer->sense();
            _origin = Timekeeper::now();
            if (!_thread && !_interesteds.empty()) {
                process(RESPOND);
                db<SmartData>(TRC) << "Sending" << endl;
            }
        } else
            db<SmartData>(WRN) << "SmartData[R]::operator= called for sensing-only transducer!" << endl;

        return *this;
    }

    const Power_Mode &power() const { return Transducer::power(); }
    void power(const Power_Mode &mode) const { Transducer::power(mode); }

    DB_Record db_record() {
        Global_Space gs = where();
        Time abs_time   = when();
        DB_Record record;
        record.type        = STATIC; // FIXME: get from version?
        record.unit        = UNIT;   // TODO: get from current value
        record.value       = _value;
        record.uncertainty = _uncertainty;
        record.x           = gs.x();
        record.y           = gs.y();
        record.z           = gs.z();
        record.t           = abs_time;
        record.device      = _device;
        return record;
    }

    static Space here() { return Locator::here(); } // Scale conversion done by SmartData::_Space specializations
    static Time now() { return Timekeeper::now(); }

    friend OStream &operator<<(OStream &os, const Responsive_SmartData &d) {
        os << "{RES:" << ((d._thread) ? "TT" : "ED");
        os << ':';
        os << (((d._mode & MODE_MASK) == COMMANDED) ? "CMD" : ((d._mode & ADVERTISED) ? "ADV" : "PRI"));
        os << ':';
        os << ((d._mode & CUMULATIVE) ? "S" : "I");
        os << ':';
        os << ((d._mode & PREDICTIVE) ? "P" : "A");
        if (d._thread) os << ",p=" << d._thread->period();
        os << ",u=" << d.unit() << ",d=" << d._device << ",o=" << d._origin << ",v=" << d._value
           << ",err=" << int(d._uncertainty) << ",x=" << d._expiry << "}";
        return os;
    }

  private:
    void process(const Mode &op) {
        db<SmartData>(TRC) << "SmartData[R]::process(op="
                           << ((op == ADVERTISE) ? "ADV"
                               : (op == CONCEAL) ? "DEL"
                               : (op == RESPOND) ? "RES"
                                                 : "CTL")
                           << ")" << endl;

        if (_mode & ADVERTISED) {
            if (FRAGMENTS > 0) {
                Buffer *buffer;
                Header *header;
                Response *response;
                if (op != RESPOND) {
                    buffer = Network::alloc(
                        sizeof(Response),
                        0); // Advertise does not forward value, so there is no need to alloc sizeof(Value)
                    header   = buffer->frame()->template data<Header>();
                    response = new (header)
                        Response(_origin, UNIT, _device, (_mode | op), _uncertainty, _expiry, 0, _signature);
                    db<SmartData>(INF) << "SmartData[R]::process:msg=" << *response << endl;
                    Network::send(buffer);
                    return;
                }
                db<SmartData>(TRC) << "SmartData[R]::process:preparing fragmentation" << endl;

                Digital<sizeof(Value)> *value = reinterpret_cast<Digital<sizeof(Value)> *>(
                    &_value); // avoid compilation errors on accessing value as a array, this is safe once FRAGMENTS > 0
                              // is only true for Digital Data
                for (UInt32 i = 0; i < FRAGMENTS; i++) {
                    buffer   = Network::alloc(sizeof(Response), MAX_PAYLOAD_RESPONSE);
                    header   = buffer->frame()->template data<Header>();
                    response = new (header)
                        Response(_origin, UNIT, _device, (_mode | op), _uncertainty, _expiry, i, _signature);
                    db<SmartData>(INF) << "SmartData[R]::process:preparing fragmentation:before copy << buffer->size()="
                                       << buffer->size() << ",v[(i+1)*MAX_RSP_SIZE-1] is accessible?"
                                       << int(((*(value))[(i + 1) * MAX_PAYLOAD_RESPONSE - 1])) << endl;
                    response->value<Digital<MAX_PAYLOAD_RESPONSE>>(
                        *reinterpret_cast<Digital<MAX_PAYLOAD_RESPONSE> *>(&((*value)[i * MAX_PAYLOAD_RESPONSE])));

                    db<SmartData>(INF) << "SmartData[R]::process:frag[" << i << "]:msg=" << *response << endl;
                    Network::send(buffer);
                }
                if (LAST_FRAGMENT_SIZE > 0) {
                    buffer   = Network::alloc(sizeof(Response), LAST_FRAGMENT_SIZE);
                    header   = buffer->frame()->template data<Header>();
                    response = new (header)
                        Response(_origin, UNIT, _device, (_mode | op), _uncertainty, _expiry, FRAGMENTS, _signature);
                    response->value<Digital<LAST_FRAGMENT_SIZE>>(*reinterpret_cast<Digital<LAST_FRAGMENT_SIZE> *>(
                        &((*value)[FRAGMENTS * MAX_PAYLOAD_RESPONSE])));
                    db<SmartData>(INF) << "SmartData[R]::process:last_frag[" << FRAGMENTS << "]:msg=" << *response
                                       << endl;
                    Network::send(buffer);
                }
                db<SmartData>(INF) << "SmartData[R]::process:frag:finished!" << endl;
            } else {
                if (op == RESPOND) {
                    Buffer *buffer     = Network::alloc(sizeof(Response), sizeof(Value));
                    Header *header     = buffer->frame()->template data<Header>();
                    Response *response = new (header) Response(
                        _origin, UNIT, _device, (_mode | op), _uncertainty, _expiry, 0,
                        _signature); // This will overwrite TSTP Packet Id once sizeof(Response) > sizeof(Header)

                    if (op == RESPOND) response->value<Value>(_value);

                    db<SmartData>(INF) << "SmartData[R]::process:msg=" << *response << endl;
                    Network::send(buffer);
                } else {
                    Buffer *buffer = Network::alloc(
                        sizeof(Response),
                        0); // Advertise does not forward value, so there is no need to alloc sizeof(Value)
                    Header *header     = buffer->frame()->template data<Header>();
                    Response *response = new (header) Response(
                        _origin, UNIT, _device, (_mode | op), _uncertainty, _expiry, 0,
                        _signature); // This will overwrite TSTP Packet Id once sizeof(Response) > sizeof(Header)
                    db<SmartData>(INF) << "SmartData[R]::process:msg=" << *response << endl;
                    Network::send(buffer);
                }
            }
        }
        notify();
    }

    // Network::Observer::update pure virtual method, called whenever the Network receives a SmartData-related message
    void update(typename Network::Observed *obs, Buffer *buffer) {
        Header *header = buffer->frame()->template data<Header>();
        db<SmartData>(TRC) << "SmartData[R]::update(obs=" << obs << ",buf=" << buffer << ",type=" << header->type()
                           << ")" << endl;
        switch (header->type()) {
        case INTEREST: {
            Interest *interest = reinterpret_cast<Interest *>(header);
            db<SmartData>(TRC) << "SmartData[R]::update:msg=" << *interest << endl;
            // previous update was not verifying device nor containment in interest region... Unit verification was
            // removed from TSTP, so it was added here
            if ((_mode & ADVERTISED) && (interest->unit() == Unit(UNIT)) && (interest->device() == _device)) {
                if (interest->mode() & REVOKE)
                    unbind(interest);
                else {
                    bind(interest);
                    db<SmartData>(INF) << "BINDED => SmartData[R]::update:msg=" << *interest << endl;
                }
                // if(!_interesteds.empty()) {
                //     if(!active) {
                //         _value = _transducer->sense();
                //         _origin = Timekeeper::now();
                //     }
                //     process(RESPOND);
                // }
            } else
                db<SmartData>(TRC) << "SmartData[R]::update: not advertised or not interested!" << endl;
        } break;
        case RESPONSE: {
            Response *response = reinterpret_cast<Response *>(header);
            db<SmartData>(TRC) << "SmartData[R]::update:msg=" << *response << endl;
            db<SmartData>(INF) << "SmartData[R]::update: not interested!" << endl;
        } break;
        case COMMAND: {
            Command *command = reinterpret_cast<Command *>(header);
            db<SmartData>(TRC) << "SmartData[R]::update:msg=" << *command << endl;

            if ((_mode & MODE_MASK) == COMMANDED) {
                _transducer->actuate(command->template value<Value>());
                _value = _transducer->sense();
            } else
                db<SmartData>(INF) << "SmartData[R]::update: not commanded!" << endl;
        } break;
        case CONTROL: {
            Control *control = reinterpret_cast<Control *>(header);
            db<SmartData>(TRC) << "SmartData[R]::update:msg=" << *control << endl;

            //            switch(header->mode()) {
            //            case Network::MODEL: {
            //                Network::Model * model = reinterpret_cast<Network::Model *>(packet);
            //                if(model->model<Model_Common>()->type() == Predictor::Model::TYPE) {
            //                    if(_predictor){
            //                        _origin = model->time();
            //                        _error = model->error();
            //                        _coordinates = model->origin();
            //                        _predictor->update(*model->model<typename Predictor::Model>(), false);
            //                        _value = _predictor->predict(_origin.t);
            //                        notify();
            //                    }
            //                }
            //            } break;
            //            default:
            //                break;
            //            }
        } break;
        }
    }

    // Transducer::Observer::update (or SmartData::Observer::update) pure virtual method, called whenever the Transducer
    // gets updated (i.e. an event-driven SmartData)
    void update(typename Transducer::Observed *obs) {
        if ((Transducer::TYPE & Transducer::ACTUATOR) && !(Transducer::TYPE & Transducer::SENSOR)) {
            // this update is for sure from another SmartData I'm interested in (Local -- e.g., an interested SmartData)
            Interested_SmartData<Transducer> *r = reinterpret_cast<Interested_SmartData<Transducer> *>(
                obs); // For now, only SmartData from the same UNIT are supported (interest from x (Remote) to x
                      // Actuator -- Interested SmartData does not have a transducer by definition)
            _transducer->actuate(*r);
        } else {
            _origin       = Timekeeper::now();
            _value        = _transducer->sense();
            _uncertainty  = _transducer->uncertainty();
            Signature sig = _transducer->signature();
            if (sig != 0) _signature = sig;

            db<SmartData>(INF) << "SmartData[R]::update(this=" << this << ",x=" << _expiry << ")" << endl;
            db<SmartData>(INF) << "_interesteds empty?" << _interesteds.empty() << endl;
            if (!_thread && !_interesteds.empty())
                process(RESPOND);
            else
                notify();
        }
    }

    bool bind(Interest *interest) {
        db<SmartData>(TRC) << "SmartData[R]::bind(int=" << interest << ")" << endl;

        bool bound                       = false;
        typename Interesteds::Iterator i = _interesteds.begin();
        for (; i != _interesteds.end(); i++)
            if ((interest->device() == _device) && i->object()->region().contains(interest->region())) break;

        if (i == _interesteds.end()) {
            Binding *binding = new /*(SYSTEM)*/ Binding(*interest);
            _interesteds.insert(binding->link());
            if (interest->period()) {
                if (!_thread) {
                    struct arg_struct_updater *args =
                        reinterpret_cast<struct arg_struct_updater *>(malloc(sizeof(struct arg_struct_updater)));
                    args->sd = this;
                    _thread  = new /*(SYSTEM)*/ Periodic_Thread(QUARK::Microsecond(interest->period()), &updater,
                                                                reinterpret_cast<void *>(args));
                } else {
                    Microsecond min = interest->period() > interest->expiry() ? interest->expiry() : interest->period();
                    if (min != _thread->period()) _thread->period(Math::gcd(_thread->period(), min));
                }
            }
            bound = true;
        }

        db<SmartData>(INF) << "SmartData[R]::bind:" << (bound ? "bound" : "not bound") << "!" << endl;

        return bound;
    }

    bool unbind(Interest *interest) {
        bool bound = true;

        typename Interesteds::Iterator i = _interesteds.begin();
        for (; i != _interesteds.end(); i++)
            if ((interest->device() == _device) && (i->object()->region() == interest->region())) break;

        if (i != _interesteds.end()) {
            _interesteds.remove(i);
            delete i->object();
            if (_interesteds.empty()) {
                if (_thread) {
                    delete _thread;
                    _thread = 0;
                }
                if (_predictor) {
                    delete _predictor;
                    _predictor = 0;
                }
                bound = false;
            }
        }

        return bound;
    }

    // Time-triggered updater
    struct arg_struct_updater {
        Responsive_SmartData *sd;
    };

    static void *updater(void *args) {
        // UInt32 device, Time expiry, Responsive_SmartData * sd) {
        // Thread::assignhandler();
        struct arg_struct_updater *arguments = static_cast<struct arg_struct_updater *>(args);
        Responsive_SmartData *sd             = arguments->sd;

        db<SmartData>(TRC) << "SmartData[R]::updater(sd=" << sd << ")" << endl;
        Signature sig;
        while (true) {
            sd->_value       = sd->_transducer->sense();
            sd->_origin      = Timekeeper::now(); // this leads to a fake timing if transducer is active
            sd->_uncertainty = sd->_transducer->uncertainty();
            sig              = sd->_transducer->signature();
            if (sig != 0) sd->_signature = sig;
            sd->process(RESPOND);
            Periodic_Thread::wait();
        }
        return 0;
    }

  private:
    Mode _mode;
    Spacetime _origin;
    UInt32 _device;
    Value _value;
    Uncertainty _uncertainty;
    Time _expiry;
    Signature _signature;

    Transducer *_transducer;
    Predictor *_predictor;
    Periodic_Thread *_thread;

    typename Simple_List<SmartData>::Element _link;

    static Interesteds _interesteds;
    static Responsives _responsives;
};

// SmartData encapsulating remote transducers
template <typename _Unit, typename Network>
class Interested_SmartData : public SmartData, public Observed, private Network::Observer {
  public:
    static const UInt32 UNIT     = _Unit::UNIT;
    static const bool predictive = (Traits<SmartData>::PREDICTOR != Traits<SmartData>::NONE);

    typedef typename Unit::Get<UNIT>::Type Value;
    static const UInt32 MAX_PAYLOAD_RESPONSE = (Network::Packet::MTU - sizeof(Response)) / 4;
    static const UInt16 FRAGMENTS            = sizeof(Value) / MAX_PAYLOAD_RESPONSE;
    static const UInt32 LAST_FRAGMENT_SIZE   = sizeof(Value) - (FRAGMENTS * MAX_PAYLOAD_RESPONSE);
    typedef SmartData::Observed<Network> Observed;
    typedef SmartData::Observer<Network> Observer;
    using SmartData::Header;
    using SmartData::Interest;
    using Space = SmartData::Global_Space;

  private:
    typedef typename Network::Buffer Buffer;
    typedef typename Network::Locator Locator;
    typedef typename Network::Timekeeper Timekeeper;
    typedef typename Select_Predictor<Traits<SmartData>::PREDICTOR>::template Predictor<Time, Value> Predictor;

    typedef Simple_List<SmartData> Interests;

    enum Operation { ANNOUNCE, SUPPRESS, COMMAND, CONTROL };

  public:
    Interested_SmartData(const Region &region,
                         Time expiry,
                         Microsecond period      = 0,
                         Mode mode               = SINGLE,
                         Uncertainty uncertainty = ANY,
                         Device_Id device        = UNIQUE,
                         Time_Offset timeout     = 0)
        : _mode(mode),
          _region(region),
          _device(device),
          _uncertainty(uncertainty),
          _expiry(expiry),
          _period(period),
          _predictor((predictive && (mode & PREDICTIVE)) ? new /*(SYSTEM)*/ Predictor : 0),
          _value(0),
          _fragments(0),
          _timeout(timeout),
          _started_update(0),
          _link(this) {
        db<SmartData>(TRC) << "SmartData[I](r=" << region << ",d=" << device << ",x=" << expiry
                           << ",m=" << ((mode & ALL) ? "ALL" : "SGL") << ",err=" << int(uncertainty) << ",p=" << period
                           << ")=>" << this << endl;
        _interests.insert(&_link);
        Network::attach(this);
        process(ANNOUNCE);
        db<SmartData>(INF) << "SmartData[I]::this=" << this << "=>" << *this << endl;
    }

    virtual ~Interested_SmartData() {
        db<SmartData>(TRC) << "~SmartData[I](this=" << this << ")" << endl;
        process(SUPPRESS);
        Network::detach(this);
        _interests.remove(&_link);
    }

    const Unit unit() const { return UNIT; }

    const Mode &interest_mode() const { return _mode; }
    const Uncertainty &interest_uncertainty() const { return _uncertainty; }

    const Mode &mode() const { return _response.mode(); }
    const Uncertainty &uncertainty() const { return _response.uncertainty(); }

    Space where() const { return Locator::absolute(_response.origin().space); }
    Time when() const { return Timekeeper::absolute(_response.origin().time); }

    Time expiry() const { return _response.expiry(); }
    bool expired() const { return Timekeeper::now() > (_response.time() + _expiry); }

    operator Value &() {
        db<SmartData>(TRC) << "SmartData[I]::operator Value()[v=" << _value << "]" << endl;
        if (expired()) {
            if (predictive)
                _value = _predictor->predict(Timekeeper::now());
            else
                // Remote data sources should have sent messages timely, thus triggering update()
                db<SmartData>(WRN) << "SmartData[I]::value(this=" << this << ",t=" << _response.time() + _expiry
                                   << ",v=" << _value << ") => expired!" << endl;
        }
        return _value;
    }

    SmartData &operator=(const Value &v) {
        process(COMMAND, v);
        return *this;
    }

    //    const Power_Mode & power() const { return Transducer::power(); } distributed power management coming soon!
    //    void power(const Power_Mode & mode) const { Transducer::power(mode); }

    DB_Record db_record() {
        Global_Space gs = where();
        Time abs_time   = when();
        DB_Record record;
        record.type        = _response.version(); // TODO: get from version
        record.unit        = _response.unit();    // get from current reponse once unit may vary for some MSD
        record.value       = _value;
        record.uncertainty = _response.uncertainty();
        record.x           = gs.x();
        record.y           = gs.y();
        record.z           = gs.z();
        record.t           = abs_time;
        record.device      = _response.device();
        return record;
    }

    DB_Series db_series() {
        DB_Series series;
        series.type = STATIC;
        series.unit = _response.unit(); // get from current reponse once unit may vary for some MSD
        Space c     = Locator::absolute(_region.center);
        series.x    = c.x();
        series.y    = c.y();
        series.z    = c.z();
        series.r    = _region.radius();
        series.t0   = Timekeeper::absolute(_region.t0);
        series.t1   = Timekeeper::absolute(_region.t1);
        return series;
    }

    static Space here() { return Locator::here(); } // Scale conversion done by SmartData::_Space specializations
    static Time now() { return Timekeeper::now(); }

    friend OStream &operator<<(OStream &os, const Interested_SmartData &d) {
        os << "{INT:" << ((d._period) ? "TT" : "ED");
        os << ':';
        os << ((d._mode & ALL) ? "ALL" : "SGL");
        os << ",r=" << d._region << ",d=" << d._device << ",err=" << int(d._uncertainty) << ",x=" << d._expiry;
        if (d._period) os << ",p=" << d._period;
        os << ",res=" << d._response << "}}";
        return os;
    }

  private:
    void process(const Operation &op, const Value &v = 0) {
        db<SmartData>(TRC) << "SmartData[I]::process(op="
                           << ((op == ANNOUNCE)   ? "ANN"
                               : (op == SUPPRESS) ? "SUP"
                               : (op == COMMAND)  ? "COM"
                                                  : "CTL")
                           << ",v=" << v << ")" << endl;
        Buffer *buffer;

        if (op == COMMAND) {
            buffer           = Network::alloc(sizeof(Command), sizeof(Value));
            Header *header   = buffer->frame()->template data<Header>();
            Command *command = new (header) Command(_region, UNIT, _device, (_mode | op), _expiry, _period);
            command->type(COMMAND);
            command->value<Value>(v);
            db<SmartData>(INF) << "SmartData[C]::process:msg=" << *command << endl;
        } else {
            buffer         = Network::alloc(sizeof(Interest));
            Header *header = buffer->frame()->template data<Header>();
            Interest *interest =
                new (header) Interest(_region, UNIT, _device, (_mode | op), _uncertainty, _expiry, _period);
            db<SmartData>(INF) << "SmartData[I]::process:msg=" << *interest << endl;
        }

        Network::send(buffer);
    }

    // Network::Observer::update pure virtual method, called whenever the Network receives a SmartData-related message
    void update(typename Network::Observed *obs, Buffer *buffer) {
        db<SmartData>(TRC) << "SmartData[I]::update(obs=" << obs << ",buf=" << buffer << ")" << endl;
        Header *header = buffer->frame()->template data<Header>();
        switch (header->type()) {
        case INTEREST: {
            Interest *interest = buffer->frame()->template data<Interest>();
            db<SmartData>(INF) << "SmartData[I]::update:msg=" << *interest << endl;
            db<SmartData>(WRN) << "SmartData[I]::update:not advertised!" << endl;
        } break;
        case RESPONSE: {
            Response *response = buffer->frame()->template data<Response>();
            db<SmartData>(TRC) << "SmartData[I]::update:msg=" << *response << endl;
            if ((response->unit() == Unit(UNIT)) && _region.contains(response->origin()) &&
                (response->device() == _device)) {
                if ((response->operation()) == ADVERTISE)
                    process(ANNOUNCE);
                else {
                    _response = *response;
                    if (FRAGMENTS > 0) {
                        db<SmartData>(INF)
                            << "SmartData[I]::update:receiving fragment=" << _response.fragment_index() << endl;
                        // if (_fragments == 0) {
                        //     _started_update = now();
                        // } else {
                        //     Time t_now = now();
                        //     if ( (t_now - _started_update) > _timeout) {
                        //         db<SmartData>(ERR) << "SmartData[I]::update:Timeout<" << _timeout << ">,start=" <<
                        //         _started_update << ",now=" << t_now << ",received_frags=" << _fragments <<
                        //         ",expected=" << (LAST_FRAGMENT_SIZE == 0 ? FRAGMENTS-1 : FRAGMENTS) << endl;
                        //         _started_update = t_now;
                        //         _fragments = 0;
                        //     }
                        // }
                        Digital<sizeof(Value)> *value = reinterpret_cast<Digital<sizeof(Value)> *>(
                            &_value); // avoid compilation errors on accessing value as a array, this is safe once
                                      // FRAGMENTS > 0 is only true for Digital Data
                        if ((LAST_FRAGMENT_SIZE > 0 &&
                             _response.fragment_index() ==
                                 FRAGMENTS)) { // last fragment (if LAST_FRAMENT_SIZE == 0, then last fragment has
                                               // payload equal to MAX_PAYLOAD_RESPONSE)
                            if (_mode & CUMULATIVE)
                                *reinterpret_cast<Digital<LAST_FRAGMENT_SIZE> *>(
                                    &((*value)[FRAGMENTS * MAX_PAYLOAD_RESPONSE])) +=
                                    response->template value<Digital<LAST_FRAGMENT_SIZE>>();
                            else
                                *reinterpret_cast<Digital<LAST_FRAGMENT_SIZE> *>(
                                    &((*value)[FRAGMENTS * MAX_PAYLOAD_RESPONSE])) =
                                    response->template value<Digital<LAST_FRAGMENT_SIZE>>();
                        } else {
                            if (_mode & CUMULATIVE)
                                *reinterpret_cast<Digital<MAX_PAYLOAD_RESPONSE> *>(
                                    &((*value)[_response.fragment_index() * MAX_PAYLOAD_RESPONSE])) +=
                                    response->template value<Digital<MAX_PAYLOAD_RESPONSE>>();
                            else
                                *reinterpret_cast<Digital<MAX_PAYLOAD_RESPONSE> *>(
                                    &((*value)[_response.fragment_index() * MAX_PAYLOAD_RESPONSE])) =
                                    response->template value<Digital<MAX_PAYLOAD_RESPONSE>>();
                        }
                        db<SmartData>(INF) << "SmartData[I]::update:copied fragment!" << endl;
                        _fragments++;
                        if ((LAST_FRAGMENT_SIZE > 0 && _response.fragment_index() == FRAGMENTS) ||
                            (LAST_FRAGMENT_SIZE == 0 &&
                             _response.fragment_index() ==
                                 FRAGMENTS - 1)) { // unordered receive support + lost packet handling via timeout
                            db<SmartData>(LOGGER)
                                << "SmartData[I]::update:received all fragments=" << _response.fragment_index() << endl;
                            _fragments = 0;
                            notify();
                        }
                    } else {
                        if (_mode & CUMULATIVE)
                            _value += response->template value<Value>();
                        else
                            _value = response->template value<Value>();
                        notify();
                    }
                }
            } else
                db<SmartData>(INF) << "SmartData[I]::update: not interested!" << endl;
        } break;
        case COMMAND: {
            Command *command = buffer->frame()->template data<Command>();
            db<SmartData>(INF) << "SmartData[I]::update:msg=" << *command << endl;
            db<SmartData>(WRN) << "SmartData[I]::update: not commanded!" << endl;
        } break;
        case CONTROL: {
            Control *control = buffer->frame()->template data<Control>();
            db<SmartData>(INF) << "SmartData[I]::update:msg=" << *control << endl;

            //            switch(header->subtype()) {
            //            case Network::MODEL: {
            //                Network::Model * model = reinterpret_cast<Network::Model *>(packet);
            //                if(model->model<Model_Common>()->type() == Predictor::Model::TYPE) {
            //                    if(_predictor){
            //                        _origin = model->time();
            //                        _error = model->error();
            //                        _coordinates = model->origin();
            //                        _predictor->update(*model->model<typename Predictor::Model>(), false);
            //                        _value = _predictor->predict(_origin.t);
            //                        notify();
            //                    }
            //                }
            //            } break;
            //            default:
            //                break;
            //            }
        } break;
        }
    }

  private:
    // Interested attributes
    Mode _mode;
    Region _region;
    UInt32 _device;
    Uncertainty _uncertainty;
    Time _expiry;
    Microsecond _period;
    Predictor *_predictor;
    Value _value; // last response value
    UInt16 _fragments;
    Time_Offset _timeout;
    Time _started_update;
    typename Simple_List<SmartData>::Element _link;
    Response _response;

    static Interests _interests;
};

// SmartData encapsulating controllers
template <typename Unit, typename Network> class Controller_SmartData : public SmartData, public Observed {
  private:
  public:
    template <typename Controller, typename... SD> Controller_SmartData(Controller controller, SD... sd);

    virtual ~Controller_SmartData();
};

template <typename Transducer, typename Network>
typename Responsive_SmartData<Transducer, Network>::Interesteds Responsive_SmartData<Transducer, Network>::_interesteds;
template <typename Transducer, typename Network>
typename Responsive_SmartData<Transducer, Network>::Responsives Responsive_SmartData<Transducer, Network>::_responsives;

template <typename Unit, typename Network>
typename Interested_SmartData<Unit, Network>::Interests Interested_SmartData<Unit, Network>::_interests;

#include <transducer.h>

#endif
