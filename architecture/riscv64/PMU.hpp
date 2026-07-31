#ifndef __QUARK_RISCV64_PMU__
#define __QUARK_RISCV64_PMU__

#include <Traits.hpp>

namespace QUARK {

class PMU {
    static void enable(size_t channel) {
        uint64_t mask = 1ULL << channel;
        asm volatile("csrc mcountinhibit, %0" ::"r"(mask));
    }

    static void disable(size_t channel) {
        uint64_t mask = 1ULL << channel;
        asm volatile("csrs mcountinhibit, %0" ::"r"(mask));
    }

    static void configure(size_t channel, uint64_t event) {
        switch (channel) {
            case 3: asm volatile("csrw mhpmevent3, %0" ::"r"(event)); break;
            case 4: asm volatile("csrw mhpmevent4, %0" ::"r"(event)); break;
            case 5: asm volatile("csrw mhpmevent5, %0" ::"r"(event)); break;
            case 6: asm volatile("csrw mhpmevent6, %0" ::"r"(event)); break;
            case 7: asm volatile("csrw mhpmevent7, %0" ::"r"(event)); break;
            case 8: asm volatile("csrw mhpmevent8, %0" ::"r"(event)); break;
            case 9: asm volatile("csrw mhpmevent9, %0" ::"r"(event)); break;
            case 10: asm volatile("csrw mhpmevent10, %0" ::"r"(event)); break;
            case 11: asm volatile("csrw mhpmevent11, %0" ::"r"(event)); break;
            case 12: asm volatile("csrw mhpmevent12, %0" ::"r"(event)); break;
            case 13: asm volatile("csrw mhpmevent13, %0" ::"r"(event)); break;
            case 14: asm volatile("csrw mhpmevent14, %0" ::"r"(event)); break;
            case 15: asm volatile("csrw mhpmevent15, %0" ::"r"(event)); break;
            case 16: asm volatile("csrw mhpmevent16, %0" ::"r"(event)); break;
            case 17: asm volatile("csrw mhpmevent17, %0" ::"r"(event)); break;
            case 18: asm volatile("csrw mhpmevent18, %0" ::"r"(event)); break;
            case 19: asm volatile("csrw mhpmevent19, %0" ::"r"(event)); break;
            case 20: asm volatile("csrw mhpmevent20, %0" ::"r"(event)); break;
            case 21: asm volatile("csrw mhpmevent21, %0" ::"r"(event)); break;
            case 22: asm volatile("csrw mhpmevent22, %0" ::"r"(event)); break;
            case 23: asm volatile("csrw mhpmevent23, %0" ::"r"(event)); break;
            case 24: asm volatile("csrw mhpmevent24, %0" ::"r"(event)); break;
            case 25: asm volatile("csrw mhpmevent25, %0" ::"r"(event)); break;
            case 26: asm volatile("csrw mhpmevent26, %0" ::"r"(event)); break;
            case 27: asm volatile("csrw mhpmevent27, %0" ::"r"(event)); break;
            case 28: asm volatile("csrw mhpmevent28, %0" ::"r"(event)); break;
            case 29: asm volatile("csrw mhpmevent29, %0" ::"r"(event)); break;
            case 30: asm volatile("csrw mhpmevent30, %0" ::"r"(event)); break;
            case 31: asm volatile("csrw mhpmevent31, %0" ::"r"(event)); break;
            default: break;
        }
    }

    static uint64_t read(size_t channel) {
        uint64_t value = 0;
        switch (channel) {
            case 3: asm volatile("csrr %0, mhpmcounter3" : "=r"(value)); break;
            case 4: asm volatile("csrr %0, mhpmcounter4" : "=r"(value)); break;
            case 5: asm volatile("csrr %0, mhpmcounter5" : "=r"(value)); break;
            case 6: asm volatile("csrr %0, mhpmcounter6" : "=r"(value)); break;
            case 7: asm volatile("csrr %0, mhpmcounter7" : "=r"(value)); break;
            case 8: asm volatile("csrr %0, mhpmcounter8" : "=r"(value)); break;
            case 9: asm volatile("csrr %0, mhpmcounter9" : "=r"(value)); break;
            case 10: asm volatile("csrr %0, mhpmcounter10" : "=r"(value)); break;
            case 11: asm volatile("csrr %0, mhpmcounter11" : "=r"(value)); break;
            case 12: asm volatile("csrr %0, mhpmcounter12" : "=r"(value)); break;
            case 13: asm volatile("csrr %0, mhpmcounter13" : "=r"(value)); break;
            case 14: asm volatile("csrr %0, mhpmcounter14" : "=r"(value)); break;
            case 15: asm volatile("csrr %0, mhpmcounter15" : "=r"(value)); break;
            case 16: asm volatile("csrr %0, mhpmcounter16" : "=r"(value)); break;
            case 17: asm volatile("csrr %0, mhpmcounter17" : "=r"(value)); break;
            case 18: asm volatile("csrr %0, mhpmcounter18" : "=r"(value)); break;
            case 19: asm volatile("csrr %0, mhpmcounter19" : "=r"(value)); break;
            case 20: asm volatile("csrr %0, mhpmcounter20" : "=r"(value)); break;
            case 21: asm volatile("csrr %0, mhpmcounter21" : "=r"(value)); break;
            case 22: asm volatile("csrr %0, mhpmcounter22" : "=r"(value)); break;
            case 23: asm volatile("csrr %0, mhpmcounter23" : "=r"(value)); break;
            case 24: asm volatile("csrr %0, mhpmcounter24" : "=r"(value)); break;
            case 25: asm volatile("csrr %0, mhpmcounter25" : "=r"(value)); break;
            case 26: asm volatile("csrr %0, mhpmcounter26" : "=r"(value)); break;
            case 27: asm volatile("csrr %0, mhpmcounter27" : "=r"(value)); break;
            case 28: asm volatile("csrr %0, mhpmcounter28" : "=r"(value)); break;
            case 29: asm volatile("csrr %0, mhpmcounter29" : "=r"(value)); break;
            case 30: asm volatile("csrr %0, mhpmcounter30" : "=r"(value)); break;
            case 31: asm volatile("csrr %0, mhpmcounter31" : "=r"(value)); break;
        }
        return value;
    }

    static void write(size_t channel, uint64_t value) {
        switch (channel) {
            case 3: asm volatile("csrw mhpmcounter3, %0" ::"r"(value)); break;
            case 4: asm volatile("csrw mhpmcounter4, %0" ::"r"(value)); break;
            case 5: asm volatile("csrw mhpmcounter5, %0" ::"r"(value)); break;
            case 6: asm volatile("csrw mhpmcounter6, %0" ::"r"(value)); break;
            case 7: asm volatile("csrw mhpmcounter7, %0" ::"r"(value)); break;
            case 8: asm volatile("csrw mhpmcounter8, %0" ::"r"(value)); break;
            case 9: asm volatile("csrw mhpmcounter9, %0" ::"r"(value)); break;
            case 10: asm volatile("csrw mhpmcounter10, %0" ::"r"(value)); break;
            case 11: asm volatile("csrw mhpmcounter11, %0" ::"r"(value)); break;
            case 12: asm volatile("csrw mhpmcounter12, %0" ::"r"(value)); break;
            case 13: asm volatile("csrw mhpmcounter13, %0" ::"r"(value)); break;
            case 14: asm volatile("csrw mhpmcounter14, %0" ::"r"(value)); break;
            case 15: asm volatile("csrw mhpmcounter15, %0" ::"r"(value)); break;
            case 16: asm volatile("csrw mhpmcounter16, %0" ::"r"(value)); break;
            case 17: asm volatile("csrw mhpmcounter17, %0" ::"r"(value)); break;
            case 18: asm volatile("csrw mhpmcounter18, %0" ::"r"(value)); break;
            case 19: asm volatile("csrw mhpmcounter19, %0" ::"r"(value)); break;
            case 20: asm volatile("csrw mhpmcounter20, %0" ::"r"(value)); break;
            case 21: asm volatile("csrw mhpmcounter21, %0" ::"r"(value)); break;
            case 22: asm volatile("csrw mhpmcounter22, %0" ::"r"(value)); break;
            case 23: asm volatile("csrw mhpmcounter23, %0" ::"r"(value)); break;
            case 24: asm volatile("csrw mhpmcounter24, %0" ::"r"(value)); break;
            case 25: asm volatile("csrw mhpmcounter25, %0" ::"r"(value)); break;
            case 26: asm volatile("csrw mhpmcounter26, %0" ::"r"(value)); break;
            case 27: asm volatile("csrw mhpmcounter27, %0" ::"r"(value)); break;
            case 28: asm volatile("csrw mhpmcounter28, %0" ::"r"(value)); break;
            case 29: asm volatile("csrw mhpmcounter29, %0" ::"r"(value)); break;
            case 30: asm volatile("csrw mhpmcounter30, %0" ::"r"(value)); break;
            case 31: asm volatile("csrw mhpmcounter31, %0" ::"r"(value)); break;
        }
    }

    static uint64_t event(size_t channel) {
        uint64_t value = 0;
        switch (channel) {
            case 3: asm volatile("csrr %0, mhpmevent3" : "=r"(value)); break;
            case 4: asm volatile("csrr %0, mhpmevent4" : "=r"(value)); break;
            case 5: asm volatile("csrr %0, mhpmevent5" : "=r"(value)); break;
            case 6: asm volatile("csrr %0, mhpmevent6" : "=r"(value)); break;
            case 7: asm volatile("csrr %0, mhpmevent7" : "=r"(value)); break;
            case 8: asm volatile("csrr %0, mhpmevent8" : "=r"(value)); break;
            case 9: asm volatile("csrr %0, mhpmevent9" : "=r"(value)); break;
            case 10: asm volatile("csrr %0, mhpmevent10" : "=r"(value)); break;
            case 11: asm volatile("csrr %0, mhpmevent11" : "=r"(value)); break;
            case 12: asm volatile("csrr %0, mhpmevent12" : "=r"(value)); break;
            case 13: asm volatile("csrr %0, mhpmevent13" : "=r"(value)); break;
            case 14: asm volatile("csrr %0, mhpmevent14" : "=r"(value)); break;
            case 15: asm volatile("csrr %0, mhpmevent15" : "=r"(value)); break;
            case 16: asm volatile("csrr %0, mhpmevent16" : "=r"(value)); break;
            case 17: asm volatile("csrr %0, mhpmevent17" : "=r"(value)); break;
            case 18: asm volatile("csrr %0, mhpmevent18" : "=r"(value)); break;
            case 19: asm volatile("csrr %0, mhpmevent19" : "=r"(value)); break;
            case 20: asm volatile("csrr %0, mhpmevent20" : "=r"(value)); break;
            case 21: asm volatile("csrr %0, mhpmevent21" : "=r"(value)); break;
            case 22: asm volatile("csrr %0, mhpmevent22" : "=r"(value)); break;
            case 23: asm volatile("csrr %0, mhpmevent23" : "=r"(value)); break;
            case 24: asm volatile("csrr %0, mhpmevent24" : "=r"(value)); break;
            case 25: asm volatile("csrr %0, mhpmevent25" : "=r"(value)); break;
            case 26: asm volatile("csrr %0, mhpmevent26" : "=r"(value)); break;
            case 27: asm volatile("csrr %0, mhpmevent27" : "=r"(value)); break;
            case 28: asm volatile("csrr %0, mhpmevent28" : "=r"(value)); break;
            case 29: asm volatile("csrr %0, mhpmevent29" : "=r"(value)); break;
            case 30: asm volatile("csrr %0, mhpmevent30" : "=r"(value)); break;
            case 31: asm volatile("csrr %0, mhpmevent31" : "=r"(value)); break;
        }
        return value;
    }

  public:
    static inline uint64_t cycles() {
        uint64_t event;
        asm volatile("rdcycle %0" : "=r"(event));
        return event;
    }

    static inline uint64_t instret() {
        uint64_t event;
        asm volatile("rdinstret %0" : "=r"(event));
        return event;
    }

    void load() {
        if constexpr (Traits<PMU>::Enable) {
            if (!initialized_) {
                initialized_ = true;
                for (size_t channel = 3; channel < 3 + Traits<PMU>::Programmable; ++channel) {
                    disable(channel);
                    configure(channel, 0);
                    write(channel, 0);
                }
            } else {
                for (size_t channel = 3; channel < 3 + Traits<PMU>::Programmable; ++channel) {
                    configure(channel, mhpmevent_[channel - 3]);
                    write(channel, mhpmcounter_[channel - 3]);
                    if (!(mcountinhibit_ & (1ULL << channel)))
                        enable(channel);
                    else
                        disable(channel);
                }
            }
        }
    }

    void save() {
        if constexpr (Traits<PMU>::Enable) {
            mcountinhibit_ = csrr<MachineMode::MCOUNTINHIBIT>();
            for (size_t channel = 3; channel < 3 + Traits<PMU>::Programmable; ++channel) {
                mhpmevent_[channel - 3]   = event(channel);
                mhpmcounter_[channel - 3] = read(channel);
            }
        }
    }

  private:
    Meta::IF<Traits<PMU>::Enable, bool, Meta::Empty>::Result initialized_;
    Meta::IF<Traits<PMU>::Enable, uint64_t, Meta::Empty>::Result mcountinhibit_;
    Meta::IF<Traits<PMU>::Enable, Meta::Array<Traits<PMU>::Programmable, uint64_t>, Meta::Empty>::Result mhpmevent_;
    Meta::IF<Traits<PMU>::Enable, Meta::Array<Traits<PMU>::Programmable, uint64_t>, Meta::Empty>::Result mhpmcounter_;
};

} // namespace QUARK

#endif
