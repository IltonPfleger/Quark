#pragma once
#include <drivers/i2c/I2C_Controller.hpp>
#include <drivers/pmic/PMIC_Controller.hpp>

namespace QUARK {

template <typename Tag> class AXP15060_Controller : public PMIC_Controller {
    using Traits = QUARK::Traits<Tag>;

    enum { DCDC1_VOLTAGE_CONTROL = 0x13 };

  public:
    AXP15060_Controller(I2C_Controller &i2c)
        : i2c_(i2c) {}

    virtual bool voltage(unsigned int rail, uint32_t microvolts) {
        bool found         = false;
        uint32_t milivolts = microvolts / 1000;

        for (const auto &i : Traits::Voltages) {
            if (i[0] == rail) {
                found = true;
                if (microvolts < i[1] || microvolts > i[2]) {
                    return false;
                }
                break;
            }
        }

        if (!found) return false;

        uint8_t data;
        if (milivolts <= 1210) {
            data = (milivolts - 500) / 10;
        } else {
            data = 71 + ((milivolts - 1220) / 20);
        }

        uint8_t payload[2];

        payload[0] = DCDC1_VOLTAGE_CONTROL + rail;
        payload[1] = data;

        i2c_.write(Traits::Address, payload, 2, true);

        return true;
    }

    virtual uint32_t voltage(unsigned int rail) {
        uint8_t target = DCDC1_VOLTAGE_CONTROL + rail;
        uint8_t data   = 0;
        uint32_t milivolts;

        if (!i2c_.write(Traits::Address, &target, 1, false)) return 0;

        if (!i2c_.read(Traits::Address, &data, 1, true)) return 0;

        unsigned int value = data & 0x7F;

        if (value < 71) {
            milivolts = 500 + (value * 10);
        } else {
            milivolts = 1220 + ((value - 71) * 20);
        }

        return milivolts * 1000;
    }

  private:
    I2C_Controller &i2c_;
};

} // namespace QUARK
