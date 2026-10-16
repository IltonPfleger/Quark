#pragma once

#include <Traits.hpp>
#include <architecture/ContextFrame.hpp>
#include <architecture/common/include/Timer.hpp>

namespace QUARK {

class Timer : public Architecture::Timer {
public:
  class Delay {
  public:
    template <typename T> Delay(T delta) {
      Microsecond elapsed = now() + delta;
      while (now() < elapsed)
        ;
    }
  };

  static Microsecond now();
  static void init();

private:
  static Microsecond us(uintmax_t);
  static void handler(ContextFrame *);
};

} // namespace QUARK
