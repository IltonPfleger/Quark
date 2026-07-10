#pragma once

#include <Alarm.hpp>
#include <architecture/Timer.hpp>
#include <types.hpp>

namespace QUARK {

class Timeout {
  public:
    Timeout(Microsecond delta)
        : at_(Timer::now() + delta) {
        Alarm(at_);
    }

    // operator bool() { return Timer::now() >= at_; }

  private:
    Alarm alarm_;
};

} // namespace QUARK
