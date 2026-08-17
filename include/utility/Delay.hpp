#ifndef __QUARK_DELAY__
#define __QUARK_DELAY__

#include <Alarm.hpp>
#include <Timer.hpp>

namespace QUARK {

class Delay {
public:
  template <typename Unit> Delay(Unit delta) { Alarm(Timer::now() + delta); }

  template <typename Unit> Delay(Unit delta, Semaphore &semaphore) {
    Alarm(Timer::now() + delta, semaphore);
  }
};

} // namespace QUARK

#endif
