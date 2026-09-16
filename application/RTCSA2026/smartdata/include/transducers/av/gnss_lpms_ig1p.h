#pragma once
#include <transducer.h>

#include "lpms_ig1p.h"

template <typename DTYPE, lpms::INSDevice LPMS_DEVICE, UInt32 UNIT>
class LPMS_GNSS_Interface : public Transducer<UNIT>
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

    LPMS_GNSS_Interface(const UInt32 &dev) : _value(0), _client(lpms::Client::GetInstance())
    {
        _thread = new Periodic_Thread(PERIOD, &reader_thread, (void *)this);
    }

    ~LPMS_GNSS_Interface() { delete _thread; }
    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    static void *reader_thread(void *ptr)
    {
        Thread::assignhandler();
        LPMS_GNSS_Interface *transducer = (LPMS_GNSS_Interface *)ptr;
        DTYPE buffer;
        while (true) {
            if (transducer->_client.get_data(LPMS_DEVICE, &buffer)) {
                transducer->_value = *(LPMS_GNSS_Interface::Value *)&buffer;
                transducer->notify();
                Periodic_Thread::wait_next();
            };
        };
    }

    Value _value;
    Periodic_Thread *_thread;
    lpms::Client &_client;
};

template <typename T, lpms::INSDevice LPMS_DEVICE>
class LPMS_GNSS_LL : public LPMS_GNSS_Interface<T, LPMS_DEVICE, SmartData::Unit::Angle | SmartData::Unit::D64>
{
    friend Responsive_SmartData<LPMS_GNSS_LL>;
    LPMS_GNSS_LL(const UInt32 &dev) : LPMS_GNSS_Interface < T, LPMS_DEVICE, SmartData::Unit::Angle | SmartData::Unit::D64 > (dev) {}
};

template <typename T, lpms::INSDevice LPMS_DEVICE>
class LPMS_GNSS_A : public LPMS_GNSS_Interface<T, LPMS_DEVICE, SmartData::Unit::Length | SmartData::Unit::D64>
{
    friend Responsive_SmartData<LPMS_GNSS_A>;
    LPMS_GNSS_A(const UInt32 &dev) : LPMS_GNSS_Interface < T, LPMS_DEVICE, SmartData::Unit::Length | SmartData::Unit::D64 > (dev) {}
};

template <typename T, lpms::INSDevice LPMS_DEVICE>
class LPMS_Velocity : public LPMS_GNSS_Interface<T, LPMS_DEVICE, SmartData::Unit::Speed | SmartData::Unit::D64>
{
    friend Responsive_SmartData<LPMS_Velocity>;
    LPMS_Velocity(const UInt32 &dev) : LPMS_GNSS_Interface < T, LPMS_DEVICE, SmartData::Unit::Speed | SmartData::Unit::D64 > (dev) {}
};

// class LPMS_Timer : public Transducer<SmartData::Unit::Time | SmartData::Unit::I64>
//{
//     friend Responsive_SmartData<LPMS_Timer>;
//
//    public:
//     static const bool active             = true;
//     static const Uncertainty UNCERTAINTY = UNKNOWN;
//     static const Type TYPE               = SENSOR;
//     static const int PERIOD              = 200;
//
//    public:
//     LPMS_Timer(const Device_Id &dev) : _client(lpms::Client::GetInstance()), _value(0)
//     {
//         _uncertainty = UNKNOWN;
//         _thread      = new Periodic_Thread(PERIOD, &reader_thread, reinterpret_cast<void *>(this));
//     }
//
//     ~LPMS_Timer() { delete _thread; }
//     virtual Value sense() { return _value; }
//     virtual Uncertainty uncertainty() { return _uncertainty; }
//     virtual void actuate(const Value &value) { _value = value; }
//     virtual SmartData::Signature signature() { return 0; }
//
//     static void *reader_thread(void *p)
//     {
//         Thread::assignhandler();
//         LPMS_Timer *transducer = reinterpret_cast<LPMS_Timer *>(p);
//         while (true) {
//             if (transducer->_client.get_data(lpms::GNSS_Timestamp, &transducer->_value)) {
//                 transducer->notify();
//                 Periodic_Thread::wait_next();
//             }
//         };
//     }
//
//    private:
//     lpms::Client &_client;
//     Value _value;
//     Uncertainty _uncertainty;
//     Periodic_Thread *_thread;
// };

using LPMS_GNSS_Longitude = Responsive_SmartData<LPMS_GNSS_LL<double, lpms::GNSS_LONGITUDE>>;
using LPMS_GNSS_Latitude  = Responsive_SmartData<LPMS_GNSS_LL<double, lpms::GNSS_LATITUDE>>;
using LPMS_GNSS_Altitude  = Responsive_SmartData<LPMS_GNSS_A<double, lpms::GNSS_ALTITUDE>>;
using LPMS_GNSS_Velocity  = Responsive_SmartData<LPMS_Velocity<double, lpms::GNSS_VELOCITY>>;
// using LPMS_GNSS_Timer     = Responsive_SmartData<LPMS_Timer>;
