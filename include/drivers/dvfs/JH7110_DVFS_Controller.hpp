#ifndef __QUARK_JH7110_DVFS_CONTROLLER__
#define __QUARK_JH7110_DVFS_CONTROLLER__

#include <architecture/Timer.hpp>
#include <drivers/clock/JH7110_Clock_Controller.hpp>
#include <drivers/dvfs/DVFS_Controller.hpp>
#include <drivers/i2c/DesignWare_I2C_Controller.hpp>
#include <drivers/pmic/AXP15060_Controller.hpp>
#include <utility/Delay.hpp>

namespace QUARK {

class JH7110_DVFS_Controller : public DVFS_Controller {
    typedef JH7110_Clock_Controller<void> Clock_Controller;

    static inline const constinit PState States[] = {
        {1500000000, 1040000},
        {750000000, 900000},
        {500000000, 900000},
        {375000000, 900000},
    };

  public:
    JH7110_DVFS_Controller()
        : i2c_(),
          pmic_(i2c_) {}

    virtual bool set(const PState &pstate) override {
        PState selected;
        bool found = false;

        for (auto &i : States) {
            if (i.frequency == pstate.frequency && i.voltage == pstate.voltage) {
                selected = i;
                found    = true;
            }
        }

        if (!found) return false;

        uint32_t divisor = 1500000000 / selected.frequency;

        if (!pmic_.voltage(2, pstate.voltage)) return false;

        Delay(Microsecond(1));

        Clock_Controller::divide(Clock_Controller::SYSCRG_CLK_CPU_CORE, divisor);

        return true;
    }

    virtual Span<const PState> available() override { return Span(States, sizeof(States) / sizeof(PState)); }

    uintmax_t voltage() { return pmic_.voltage(2); }

    uintmax_t clock() { return 1500000000 / Clock_Controller::divisor(Clock_Controller::SYSCRG_CLK_CPU_CORE); }

  private:
    DesignWare_I2C_Controller<I2C5> i2c_;
    AXP15060_Controller<PMIC0> pmic_;
};

} // namespace QUARK

#endif
