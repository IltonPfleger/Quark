#include <Thread.hpp>
#include <machine/Machine.hpp>
#include <utility/Console.hpp>
#include <utility/Delay.hpp>

using namespace QUARK;

static constexpr Second TestDuration = 60;
static constexpr Hz Frequency = 10;
static constexpr Microsecond DelayStep = 1000000 / Frequency;
static constexpr unsigned int Steps = TestDuration * Frequency;

int main(int, char *[]) {
  Console::println("Thermometer test");
  Console::println("Duration: ", TestDuration, '\n', "Frequency: ", Frequency,
                   '\n', "Delay step: ", DelayStep, '\n', "Steps: ", Steps);

  for (unsigned int i = 0; i < Steps; i++) {
    Console::println("Temperature (mC): ", Thermometer::temperature());

    QUARK::Delay(Microsecond(DelayStep));
  }

  Console::println("\nFinished!");
}
