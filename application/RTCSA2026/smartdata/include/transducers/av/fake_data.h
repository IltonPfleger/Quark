#pragma once
#include <transducer.h>

template <UInt32 U> class Fake_Data : public Transducer<U> {
    friend Responsive_SmartData<Fake_Data<U>>;

  public:
    static const bool active                        = true;
    static const SmartData::Uncertainty UNCERTAINTY = SmartData::UNKNOWN;
    static const SmartData::Type TYPE               = 1;
    static const UInt32 PERIOD                      = 1000000;

  public:
    Fake_Data(const SmartData::Device_Id &dev)
        : _value(0) {
        _generator = new Periodic_Thread(PERIOD, &generate, reinterpret_cast<void *>(this));
    }

    ~Fake_Data() { delete _generator; }

    virtual typename Transducer<U>::Value sense() { return _value; }
    virtual SmartData::Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const typename Transducer<U>::Value &value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

  private:
    static void *generate(void *p) {
        Fake_Data<U> *transducer = reinterpret_cast<Fake_Data *>(p);
        while (true) {
            Periodic_Thread::wait();
            // SmartData::Time t = Responsive_SmartData<Fake_Data<U>>::now();
            // db<SmartData>(LOGGER) << t << endl;
            transducer->_value       = 0x0;
            transducer->_uncertainty = 0;
            transducer->notify();
        }
    }

  private:
    typename Transducer<U>::Value _value;
    SmartData::Uncertainty _uncertainty;
    Periodic_Thread *_generator;
};

template <UInt32 U> class Fake_Data_Digital : public Transducer<U> {
    friend Responsive_SmartData<Fake_Data_Digital<U>>;

  public:
    static const bool active                        = true;
    static const SmartData::Uncertainty UNCERTAINTY = SmartData::UNKNOWN;
    static const SmartData::Type TYPE               = 1;
    static const UInt32 PERIOD                      = 1000000;

  public:
    Fake_Data_Digital(const SmartData::Device_Id &dev)
        : _value(0) {
        _generator = new Periodic_Thread(PERIOD, &generate, reinterpret_cast<void *>(this));
    }

    ~Fake_Data_Digital() { delete _generator; }

    virtual typename Transducer<U>::Value sense() { return _value; }
    virtual SmartData::Uncertainty uncertainty() { return _uncertainty; }
    virtual void actuate(const typename Transducer<U>::Value &value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

  private:
    static void *generate(void *p) {
        Fake_Data_Digital<U> *transducer = reinterpret_cast<Fake_Data_Digital *>(p);
        while (true) {
            Periodic_Thread::wait();
            // SmartData::Time t = Responsive_SmartData<Fake_Data<U>>::now();
            transducer->_value[0]    = 0x1;
            transducer->_uncertainty = 0;
            transducer->notify();
            // db<SmartData>(LOGGER) << t << endl;
        }
    }

  private:
    typename Transducer<U>::Value _value;
    SmartData::Uncertainty _uncertainty;
    Periodic_Thread *_generator;
};

using Destination = Responsive_SmartData<Fake_Data_Digital<SmartData::Unit::MOTION_VECTOR_LOCAL | 15 << 16 | 1>>;
using Map =
    Responsive_SmartData<Fake_Data_Digital<SmartData::Unit::MOTION_VECTOR_GLOBAL | Traits<Project>::MAX_MAP_POINTS>>;
using Wheel_Telemetry = Responsive_SmartData<Fake_Data_Digital<SmartData::Unit::WHEEL_TELEMETRY>>;

using ETSI_CAM_Source = Responsive_SmartData<Fake_Data_Digital<SmartData::Unit::MOTION_VECTOR_GLOBAL | 1>>;
using ETSI_CPM_Source = Responsive_SmartData<Fake_Data_Digital<SmartData::Unit::MOTION_VECTOR_GLOBAL | 30>>;
#ifdef ARTERY_PROJECT
#ifdef NO_DATA_SOURCE
using Dynamics_State = Responsive_SmartData<Fake_Data_Digital<SmartData::Unit::MOTION_VECTOR_LOCAL | 12 << 16 | 1>>;
using Dynamics_State = Responsive_SmartData<Fake_Data_Digital<SmartData::Unit::MOTION_VECTOR_LOCAL | 12 << 16 | 1>>;
using Object_Recognition_And_Tracking_Fuser =
    Responsive_SmartData<Fake_Data_Digital<SmartData::Unit::MOTION_VECTOR_LOCAL | 3 * 50>>;
#endif
#endif

using Fuel_Consumption = Responsive_SmartData<Fake_Data<SmartData::Unit::Mass_Flow | SmartData::Unit::D64>>;
using CO2_Emission     = Responsive_SmartData<Fake_Data<SmartData::Unit::Mass_Flow | SmartData::Unit::D64>>;
using Mass             = Responsive_SmartData<Fake_Data<SmartData::Unit::Mass | SmartData::Unit::F32>>;
using Odometer         = Responsive_SmartData<Fake_Data<SmartData::Unit::Length | SmartData::Unit::F32>>;
using Speed            = Responsive_SmartData<Fake_Data<SmartData::Unit::Speed | SmartData::Unit::F32>>;
using Drag             = Responsive_SmartData<Fake_Data<SmartData::Unit::Force | SmartData::Unit::F32>>;
using Engine_RPM       = Responsive_SmartData<Fake_Data<SmartData::Unit::Frequency | SmartData::Unit::F32>>;
using Gear             = Responsive_SmartData<Fake_Data<SmartData::Unit::Counter | SmartData::Unit::I32>>;
using Longitude        = Responsive_SmartData<Fake_Data<SmartData::Unit::Angle | SmartData::Unit::F32>>;
using Latitude         = Responsive_SmartData<Fake_Data<SmartData::Unit::Angle | SmartData::Unit::F32>>;
using Altitude         = Responsive_SmartData<Fake_Data<SmartData::Unit::Length | SmartData::Unit::F32>>;

using Longitudinal_Acceleration = Responsive_SmartData<Fake_Data<SmartData::Unit::Acceleration | SmartData::Unit::F32>>;
using Lateral_Acceleration      = Responsive_SmartData<Fake_Data<SmartData::Unit::Acceleration | SmartData::Unit::F32>>;
using Vertical_Acceleration     = Responsive_SmartData<Fake_Data<SmartData::Unit::Acceleration | SmartData::Unit::F32>>;

using Yaw_Rate   = Responsive_SmartData<Fake_Data<SmartData::Unit::Angular_Velocity | SmartData::Unit::F32>>;
using Pitch_Rate = Responsive_SmartData<Fake_Data<SmartData::Unit::Angular_Velocity | SmartData::Unit::F32>>;
using Roll_Rate  = Responsive_SmartData<Fake_Data<SmartData::Unit::Angular_Velocity | SmartData::Unit::F32>>;

using Yaw   = Responsive_SmartData<Fake_Data<SmartData::Unit::Angle | SmartData::Unit::F32>>;
using Pitch = Responsive_SmartData<Fake_Data<SmartData::Unit::Angle | SmartData::Unit::F32>>;
using Roll  = Responsive_SmartData<Fake_Data<SmartData::Unit::Angle | SmartData::Unit::F32>>;

using Battery_Charge = Responsive_SmartData<Fake_Data<SmartData::Unit::Percent | SmartData::Unit::F32>>;
