#pragma once
#include <transducer.h>

#include "lpms_ig1p.h"

template <typename DTYPE, lpms::INSDevice LPMS_DEVICE, UInt32 UNIT>
class LPMS_INS_Interface : public Transducer<UNIT>
{
   public:
    using Value                          = typename Transducer<UNIT>::Value;
    using Uncertainty                    = typename SmartData::Uncertainty;
    using Type                           = typename Transducer<UNIT>::Type;
    static const Uncertainty UNKNOWN     = SmartData::UNKNOWN;
    static const UInt32 SENSOR     = Transducer<UNIT>::SENSOR;
    static const bool active             = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE               = SENSOR;
    static const int PERIOD              = 200;

    LPMS_INS_Interface(const UInt32 &dev) : _value(0), _client(lpms::Client::GetInstance())
    {
        _thread = new Periodic_Thread(PERIOD, &reader_thread, (void *)this);
    }

    ~LPMS_INS_Interface() { delete _thread; }
    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    static void *reader_thread(void *ptr)
    {
        Thread::assignhandler();
        LPMS_INS_Interface *transducer = (LPMS_INS_Interface *)ptr;
        DTYPE buffer;
        while (true) {
            if (transducer->_client.get_data(LPMS_DEVICE, &buffer)) {
                transducer->_value = *(LPMS_INS_Interface::Value *)&buffer;
                transducer->notify();
                Periodic_Thread::wait_next();
            };
        };
    }

    Value _value;
    Periodic_Thread *_thread;
    lpms::Client &_client;
};

template <typename T, lpms::INSDevice device>
class LPMS_INS_Accelerometer : public LPMS_INS_Interface<T, device, SmartData::Unit::Acceleration | SmartData::Unit::F32>
{
    friend Responsive_SmartData<LPMS_INS_Accelerometer>;

   public:
    LPMS_INS_Accelerometer(const UInt32 &dev) : LPMS_INS_Interface < T, device, SmartData::Unit::Acceleration | SmartData::Unit::F32 > (dev) {}
};

template <typename T, lpms::INSDevice device>
class LPMS_INS_Gyroscope : public LPMS_INS_Interface<T, device, SmartData::Unit::Angular_Velocity | SmartData::Unit::F32>
{
    friend Responsive_SmartData<LPMS_INS_Gyroscope>;

   public:
    LPMS_INS_Gyroscope(const UInt32 &dev) : LPMS_INS_Interface < T, device, SmartData::Unit::Angular_Velocity | SmartData::Unit::F32 > (dev) {}
};

template <typename T, lpms::INSDevice device>
class LPMS_INS_Magnetometer : public LPMS_INS_Interface<T, device, SmartData::Unit::Angle | SmartData::Unit::F32>
{
    friend Responsive_SmartData<LPMS_INS_Magnetometer>;

   public:
    LPMS_INS_Magnetometer(const UInt32 &dev) : LPMS_INS_Interface < T, device, SmartData::Unit::Angle | SmartData::Unit::F32 > (dev) {}
};

using LPMS_IMU_Longitudinal_Acceleration = Responsive_SmartData<LPMS_INS_Accelerometer<float, lpms::INS_LONGITUDINAL_ACCELERATION>>;
using LPMS_IMU_Lateral_Acceleration      = Responsive_SmartData<LPMS_INS_Accelerometer<float, lpms::INS_LATERAL_ACCELERATION>>;
using LPMS_IMU_Vertical_Acceleration     = Responsive_SmartData<LPMS_INS_Accelerometer<float, lpms::INS_VERTICAL_ACCELERATION>>;
using LPMS_IMU_Yaw_Rate                  = Responsive_SmartData<LPMS_INS_Gyroscope<float, lpms::INS_YAW_RATE>>;
using LPMS_IMU_Pitch_Rate                = Responsive_SmartData<LPMS_INS_Gyroscope<float, lpms::INS_PITCH_RATE>>;
using LPMS_IMU_Roll_Rate                 = Responsive_SmartData<LPMS_INS_Gyroscope<float, lpms::INS_ROLL_RATE>>;
using LPMS_IMU_Yaw                       = Responsive_SmartData<LPMS_INS_Magnetometer<float, lpms::INS_YAW>>;
