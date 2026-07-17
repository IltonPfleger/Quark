#pragma once

#include <architecture/Timer.hpp>
#include <drivers/clock/JH7110_Clock_Controller.hpp>
#include <drivers/dvfs/DVFS_Controller.hpp>
#include <drivers/i2c/DesignWare_I2C_Controller.hpp>
#include <drivers/pmic/AXP15060_Controller.hpp>

namespace QUARK {

class JH7110_DVFS_Controller : public DVFS_Controller {
    typedef JH7110_Clock_Controller<void> Clock_Controller;

    static inline const constinit PState States[] = {
        {375000000, 800000},
        {500000000, 800000},
        {750000000, 800000},
        {1500000000, 1040000},
    };

  public:
    JH7110_DVFS_Controller()
        : i2c_(),
          pmic_(i2c_) {}

    virtual bool set(const PState &pstate) override {
        bool valid = false;

        for (const auto &i : States) {
            if (i.frequency == pstate.frequency && i.voltage == pstate.voltage) {
                valid = true;
                break;
            }
        }

        if (!valid) return false;

        if (pstate.frequency > frequency()) {
            if (!voltage(pstate.voltage)) return false;
            stabilize();
            frequency(pstate.frequency);
            stabilize();
        } else {
            frequency(pstate.frequency);
            stabilize();
            if (!voltage(pstate.voltage)) return false;
            stabilize();
        }

        return true;
    }

    virtual Span<const PState> available() override { return Span(States, sizeof(States) / sizeof(PState)); }

    uintmax_t voltage() { return pmic_.voltage(1); }

    uintmax_t frequency() { return PLL0::rate() / Clock_Controller::divisor(Clock_Controller::SYSCRG_CLK_CPU_CORE); }

  private:
    void frequency(uintmax_t frequency) {
        assert(frequency);
        assert(PLL0::rate() % frequency == 0);
        uint32_t divisor = PLL0::rate() / frequency;
        Clock_Controller::divide(Clock_Controller::SYSCRG_CLK_CPU_CORE, divisor);
    }

    bool voltage(uintmax_t microvolts) { return pmic_.voltage(1, microvolts); }

    void stabilize() { Timer::Delay(10'000); }

  private:
    DesignWare_I2C_Controller<I2C5> i2c_;
    AXP15060_Controller<PMIC0> pmic_;
};

} // namespace QUARK
