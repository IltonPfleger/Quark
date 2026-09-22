#pragma once

// EPOS Smart Transducer Declarations

#include <smartdata.h>
// #include <fstream>
// #include <string>
// #include <stdlib.h>
#include <machine/udpnic.h>

#define _UTIL

template <UInt32 _UNIT>
class Transducer : public SmartData,
                   public Observed // 'Obverser' only when implementing a transformer
{
  protected:
    typedef SmartData::Uncertainty Uncertainty;

  public:
    static const UInt32 UNIT = _UNIT;
    enum : UInt32 { SENSOR = 1 << 0, ACTUATOR = 1 << 1 };
    typedef UInt32 Type;
    static const Type TYPE = SENSOR | ACTUATOR;

    typedef typename Unit::Get<_UNIT>::Type Value;

    typedef _UTIL::Observer Observer;
    typedef _UTIL::Observed Observed;

  protected:
    Transducer() {}

  public:
    virtual ~Transducer() {}

    virtual Value sense() = 0;
    virtual Uncertainty uncertainty() { return UNKNOWN; }
    virtual void actuate(const Value &value) {}
    virtual SmartData::Signature signature() = 0;

    Power_Mode power() { return Power_Mode::FULL; }
    void power(const Power_Mode &mode) {}
};

class Dummy_Transducer : public Transducer<SmartData::Unit::Antigravity> {
    friend Responsive_SmartData<Dummy_Transducer>;

  public:
    static const bool active             = false;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE               = SENSOR | ACTUATOR;

  public:
    Dummy_Transducer(const Device_Id &dev)
        : _value(0) {}
    ~Dummy_Transducer() {}

    virtual Value sense() { return _value++; }
    virtual Uncertainty uncertainty() { return UNCERTAINTY; }
    virtual void actuate(const Value &value) { _value = value; }
    virtual SmartData::Signature signature() { return 0; }

  private:
    Value _value;
};

using Antigravity = Responsive_SmartData<Dummy_Transducer>;
using Antigravity_Proxy =
    Interested_SmartData<Dummy_Transducer::Unit::Wrap<Dummy_Transducer::UNIT>>;

#ifdef BASIC_EXAMPLE
// Sensors
#include "transducers/example/temperature_sensor.h"
// Actuators
#include "transducers/example/cooler_actuator.h"
#endif

#ifdef CARLA_V1_PROJECT
#include "transducers/old_carla_project.h"
#endif

#ifdef AV_PROJECT
// Sensors
#include "transducers/av/battery.h"
#include "transducers/av/camera.h"
#include "transducers/av/co2_emission.h"
#include "transducers/av/destination.h"
#include "transducers/av/drag.h"
#include "transducers/av/engine_rpm.h"
#include "transducers/av/etsi_cam.h"
#include "transducers/av/etsi_cpm.h"
#include "transducers/av/fuel.h"
#include "transducers/av/gear.h"
#include "transducers/av/gps.h"
#include "transducers/av/imu.h"
#include "transducers/av/lidar.h"
#include "transducers/av/map.h"
#include "transducers/av/mass.h"
#include "transducers/av/odometer.h"
#include "transducers/av/radar.h"
#include "transducers/av/speed.h"
#include "transducers/av/wheel_telemetry.h"

// Actuators
#include "transducers/av/brake.h"      // 33
#include "transducers/av/hand_brake.h" // 35
#include "transducers/av/reverse.h"    // 34
#include "transducers/av/steer.h"      // 31
#include "transducers/av/throttle.h"   // 32
#endif
