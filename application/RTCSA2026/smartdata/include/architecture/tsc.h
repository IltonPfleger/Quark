#pragma once

// EPOS IA32 Time-Stamp Counter Mediator Declarations

#include <Alarm.hpp>
#include <utility/Delay.hpp>
#include <architecture/Timer.hpp>
#include <architecture/cpu.h>
#include <architecture/tsc.h>
#include <system/types.h>

class TSC {
  public:
    typedef UInt64 Time_Stamp;

  public:
    TSC() {}

    static Hertz frequency() {
        static Hertz frequency = 0;
        if (frequency == 0) {
            TSC::Time_Stamp t0 = TSC::time_stamp();
            QUARK::Delay(QUARK::Microsecond(1000000));
            TSC::Time_Stamp t1 = TSC::time_stamp();
            frequency          = t1 - t0;
        }
        return frequency;
    }
    static PPB accuracy() { return 50; }

    static Time_Stamp time_stamp() { return QUARK::Timer::now(); }

    static void time_stamp(const Time_Stamp &ts) {
        // CPU::wrmsr(CPU::MSR_TSC, ts);
    }
};
