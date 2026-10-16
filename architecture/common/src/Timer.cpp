#include <Alarm.hpp>
#include <Thread.hpp>
#include <utility/Ticker.hpp>

namespace QUARK {

namespace Architecture {

void Timer::handler(size_t i) {
  constexpr uintmax_t Frequency = Traits<QUARK::Timer>::Frequency;
  constexpr uintmax_t AlarmFrequency = Frequency / Traits<Alarm>::Frequency;
  constexpr uintmax_t ThreadFrequency = Frequency / Traits<Thread>::Frequency;
  constexpr uintmax_t Active = Traits<QUARK::CPU>::Active;

  using ThreadTicker = Ticker<ThreadFrequency, Thread::reschedule, Active>;
  using AlarmTicker = Ticker<AlarmFrequency, Alarm::handler, Active>;

  static constinit Meta::Tuple<AlarmTicker, ThreadTicker> tickers{};

  Meta::for_each(tickers, [i](auto &ticker) { ticker.handler(i); });
}

} // namespace Architecture

} // namespace QUARK
