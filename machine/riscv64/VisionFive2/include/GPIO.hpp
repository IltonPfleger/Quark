#include <Traits.hpp>
#include <drivers/Driver.hpp>

namespace QUARK {

class GPIO : Driver {
    static const uintptr_t k_sys_iomux_base = 0x13040000;

    enum {
        DOEN_OFFSET = 0,
        DOUT_OFFSET = 0x40,
        DIN_OFFSET  = 0x80,

        DOUT_MASK = 0x7f,
        DOEN_MASK = 0x3f,
        DIN_MASK  = 0xff,
    };

  public:
    enum class OutputSignal {
        GPO_SYS_IOMUX_U0_CAN_CTRL_TXD = 0x6,
        GPO_SYS_IOMUX_U0_CAN_CTRL_STB = 0x3,
    };

    enum class InputSignal {
        GPI_SYS_IOMUX_U0_CAN_CTRL_RXD = 0x1,
    };

    static void map(OutputSignal signal, unsigned int pin) {
        auto offset = 4 * (pin / 4);
        auto shift  = 8 * (pin % 4);
        auto dout   = static_cast<int>(signal);

        Reg32(k_sys_iomux_base, DOUT_OFFSET + offset) &= ~(DOUT_MASK << shift);
        Reg32(k_sys_iomux_base, DOUT_OFFSET + offset) |= dout << shift;

        Reg32(k_sys_iomux_base, DOEN_OFFSET + offset) &= ~(DOEN_MASK << shift);
    }

    static void map(InputSignal signal, unsigned int pin) {
        auto output_offset = 4 * (pin / 4);
        auto output_shift  = 8 * (pin % 4);

        auto integer      = static_cast<unsigned int>(signal);
        auto input_offset = 4 * (integer / 4);
        auto input_shift  = 8 * (integer % 4);

        Reg32(k_sys_iomux_base, DOUT_OFFSET + output_offset) &= ~(DOUT_MASK << output_shift);
        Reg32(k_sys_iomux_base, DOEN_OFFSET + output_offset) &= ~(DOEN_MASK << output_shift);
        Reg32(k_sys_iomux_base, DOEN_OFFSET + output_offset) |= (1 << output_shift);

        Reg32(k_sys_iomux_base, DIN_OFFSET + input_offset) &= ~(DIN_MASK << input_shift);
        Reg32(k_sys_iomux_base, DIN_OFFSET + input_offset) |= (pin + 2) << input_shift;
    }
};

} // namespace QUARK
