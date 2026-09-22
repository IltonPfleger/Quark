#pragma once

#include <Traits.hpp>
#include <utility/Console.hpp>

namespace QUARK {

template <typename Tag> class JH7110_Thermometer {
private:
  enum : uint32_t { RESET = 1, POWERDOWN = 1 << 1, ENABLE = 1 << 2 };

  enum : uint32_t { OUTPUT_MASK = 0xFFF0000 };

  const static intmax_t CONVERSION_A = 237500ULL;
  const static intmax_t CONVERSION_B = 4094ULL;
  const static intmax_t CONVERSION_C = 81100ULL;

public:
  static void init() {
    if constexpr (Traits<Tag>::Enable) {
      if (CPU::id() == Traits<CPU>::BSP) {
        *Control |= POWERDOWN;
        Timer::Delay(Microsecond(10));

        *Control = 0;
        Timer::Delay(Microsecond(65));

        *Control = RESET;
        Timer::Delay(Microsecond(10));

        *Control |= ENABLE;
        Timer::Delay(Microsecond(10));
      }
    }
  }

  static Milicelsius temperature() {
    return ((*Control & OUTPUT_MASK) >> 16) * CONVERSION_A / CONVERSION_B -
           CONVERSION_C;
  }

private:
  static volatile inline uint32_t *const Control =
      reinterpret_cast<uint32_t *>(Traits<MemoryMap>::THERMOMETER);
};

} // namespace QUARK
