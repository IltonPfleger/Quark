#pragma once

#include <Alarm.hpp>
#include <Thread.hpp>
#include <Traits.hpp>
#include <utility/Ticker.hpp>

namespace QUARK {

namespace ArchitectureCommon {

class Timer {
    static constexpr uintmax_t TimerFrequency  = Traits<QUARK::Timer>::Frequency;
    static constexpr uintmax_t AlarmFrequency  = TimerFrequency / Traits<Alarm>::Frequency;
    static constexpr uintmax_t ThreadFrequency = TimerFrequency / Traits<Thread>::Frequency;
    static constexpr uintmax_t Active          = Traits<QUARK::CPU>::Active;

    using ThreadTicker = Ticker<ThreadFrequency, Thread::reschedule, Active>;
    using AlarmTicker  = Ticker<AlarmFrequency, Alarm::handler, Active>;

  protected:
    static void handler(size_t channel) {
        Meta::forEach(tickers_, [channel]<typename T>(T &ticker) {
            if constexpr (!Meta::Same<T, Meta::Empty>::Result) {
                ticker.handler(channel);
            }
        });
    }

  private:
    static inline constinit Meta::Tuple<AlarmTicker, ThreadTicker> tickers_{};
};

} // namespace ArchitectureCommon

} // namespace QUARK
