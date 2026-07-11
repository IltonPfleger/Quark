#pragma once

#include <Traits.hpp>
#include <drivers/Driver.hpp>

namespace QUARK {

class PLL0 : Driver {
  public:
    static void pd(uint32_t value) {
        Reg32(k_sys_syscon_base, 0x20) &= ~(1 << 27);
        Reg32(k_sys_syscon_base, 0x20) |= (value & 0x1) << 27;
    }

    static void prediv(uint32_t value) {
        Reg32(k_sys_syscon_base, 0x24) &= ~(0x3F);
        Reg32(k_sys_syscon_base, 0x24) |= (value & 0x3F);
    }

    static void fbdiv(uint32_t value) {
        Reg32(k_sys_syscon_base, 0x1c) &= ~(0xFFF);
        Reg32(k_sys_syscon_base, 0x1c) |= (value & 0xFFF);
    }

    static void frac(uint32_t value) {
        Reg32(k_sys_syscon_base, 0x20) &= ~(0xFFFFFF);
        Reg32(k_sys_syscon_base, 0x20) |= (value & 0xFFFFFF);
    }

    static void postdiv1(uint32_t value) {
        Reg32(k_sys_syscon_base, 0x20) &= ~(0x3 << 28);
        Reg32(k_sys_syscon_base, 0x20) |= (value & 0x3) << 28;
    }

    static void dacpd(uint32_t value) {
        Reg32(k_sys_syscon_base, 0x18) &= ~(1 << 24);
        Reg32(k_sys_syscon_base, 0x18) |= (value & 0x1) << 24;
    }

    static void dsmpd(uint32_t value) {
        Reg32(k_sys_syscon_base, 0x18) &= ~(1 << 25);
        Reg32(k_sys_syscon_base, 0x18) |= (value & 0x1) << 25;
    }

    static void rate(uintptr_t) {
        pd(1);

        dacpd(1);
        dsmpd(1);

        prediv(2);
        fbdiv(125);
        postdiv1(0);

        pd(0);
    }

    static uint64_t rate() {
        const uint64_t refclk = 24000000ULL;

        uint32_t dacpd_v, dsmpd_v;
        uint32_t prediv_v, fbdiv_v, postdiv1_v;
        uint64_t frac_v;

        dacpd_v = (Reg32(k_sys_syscon_base, 0x18) >> 24) & 0x1;
        dsmpd_v = (Reg32(k_sys_syscon_base, 0x18) >> 25) & 0x1;

        prediv_v = Reg32(k_sys_syscon_base, 0x24) & 0x3F;
        fbdiv_v  = Reg32(k_sys_syscon_base, 0x1c) & 0xFFF;

        postdiv1_v = 1 << ((Reg32(k_sys_syscon_base, 0x20) >> 28) & 0x3);

        frac_v = Reg32(k_sys_syscon_base, 0x20) & 0xFFFFFF;

        if (dacpd_v == 1 && dsmpd_v == 1) {
            return (refclk * fbdiv_v) / (prediv_v * postdiv1_v);
        }

        if (dacpd_v == 0 && dsmpd_v == 0) {
            uint64_t numerator = ((uint64_t)fbdiv_v << 24) + frac_v;

            return (refclk * numerator) / ((uint64_t)prediv_v * postdiv1_v * (1 << 24));
        }

        return 0;
    }

  private:
    static const uintptr_t k_sys_syscon_base = 0x13030000;
};

template <typename> class JH7110ClockController : Driver {
    static const uintptr_t k_sys_crg_base    = Traits<MemoryMap>::SYSCRG;
    static const uintptr_t k_sys_syscon_base = Traits<MemoryMap>::SYS_SYSCON;
    static const uintptr_t k_aon_crg_base    = Traits<MemoryMap>::AONCRG;
    static const uintptr_t k_aon_syscon_base = Traits<MemoryMap>::AON_SYSCON;

    enum { SYSCRG = 0LL << 63, AONCRG = 1LL << 63, GATE = 1 << 31 };

  public:
    enum {
        SYSCRG_CLK_CPU_ROOT                = SYSCRG | 0x000,
        SYSCRG_CLK_CPU_CORE                = SYSCRG | 0x004,
        SYSCRG_CLK_DDR_BUS                 = SYSCRG | 0x0ac,
        SYSCRG_CLK_GMAC_PHY                = SYSCRG | 0x1b8,
        SYSCRG_CLK_GMAC0_GTX               = SYSCRG | 0x1b0,
        SYSCRG_CLK_GMAC_SOURCE             = SYSCRG | 0x18c,
        SYSCRG_CLK_GMAC5_AXI64_RX_INVERTER = SYSCRG | 0x1a0,
        SYSCRG_CLK_GMAC5_AXI64_TX_INVERTER = SYSCRG | 0x1a8,
        SYSCRG_CLK_GMAC5_AXI64_AHB         = SYSCRG | 0x184,
        SYSCRG_CLK_GMAC5_AXI64_AXI         = SYSCRG | 0x188,

        SYSCRG_CLK_CAN0_CTRL_APB   = SYSCRG | 0x1cc,
        SYSCRG_CLK_CAN0_CTRL_TIMER = SYSCRG | 0x1d0,
        SYSCRG_CLK_CAN0_CTRL_CORE  = SYSCRG | 0x1d4,

        SYSCRG_CLK_RSTN_U0_CAN_CTRL_APB          = SYSCRG | (3 << 16) | 15,
        SYSCRG_CLK_RSTN_U0_CAN_CTRL_CORE         = SYSCRG | (3 << 16) | 16,
        SYSCRG_CLK_RSTN_U0_CAN_CTRL_TIMER        = SYSCRG | (3 << 16) | 17,
        SYSCRG_CLK_RSTN_U1_GMAC5_AXI64_ARESETN_I = SYSCRG | (2 << 16) | 2,
        SYSCRG_CLK_RSTN_U1_GMAC5_AXI64_HRESET_N  = SYSCRG | (2 << 16) | 3,

        AONCRG_CLK_GMAC0_AHB               = AONCRG | 0x8,
        AONCRG_CLK_GMAC0_RMII_RTX          = AONCRG | 0x10,
        AONCRG_CLK_GMAC0_AXI               = AONCRG | 0xc,
        AONCRG_CLK_GMAC0_TX                = AONCRG | 0x14,
        AONCRG_CLK_GMAC5_AXI64_TX_INVERTER = AONCRG | 0x18,

        AONCRG_CLK_RSTN_GMAC5_AXI64_AXI = AONCRG | (0 << 16) | 0,
        AONCRG_CLK_RSTN_GMAC5_AXI64_AHB = AONCRG | (0 << 16) | 1,
    };

    static void enable(uint64_t id) {
        bool aon = id & AONCRG;
        id &= ~AONCRG;
        if (aon) {
            Reg32(k_aon_crg_base, id) |= GATE;
        } else {
            Reg32(k_sys_crg_base, id) |= GATE;
        }
    }

    static uint32_t divisor(uint64_t id) {
        bool aon = id & AONCRG;
        uintptr_t address;
        id &= ~AONCRG;
        if (aon) {
            address = k_aon_crg_base;
        } else {
            address = k_sys_crg_base;
        }
        return Reg32(address, id) & ~0xFFFFFF;
    }

    static void divide(uint64_t id, uint32_t value) {
        bool aon = id & AONCRG;
        uintptr_t address;
        id &= ~AONCRG;
        if (aon) {
            address = k_aon_crg_base;
        } else {
            address = k_sys_crg_base;
        }
        Reg32(address, id) &= ~0xFFFFFF;
        Reg32(address, id) |= value;
    }

    static void invert(uint64_t id, bool value) {
        bool aon = id & AONCRG;
        uintptr_t address;
        id &= ~AONCRG;
        if (aon) {
            address = k_aon_crg_base;
        } else {
            address = k_sys_crg_base;
        }
        Reg32(address, id) &= ~(1 << 30);
        Reg32(address, id) |= value << 30;
    }

    static void multiplex(uint64_t id, unsigned int value) {
        bool aon = id & AONCRG;
        uintptr_t address;
        id &= ~AONCRG;
        if (aon) {
            address = k_aon_crg_base;
        } else {
            address = k_sys_crg_base;
        }
        Reg32(address, id) &= ~(0x3F << 24);
        Reg32(address, id) |= (value & 0x3F) << 24;
    }

    static void reset(uint64_t id) {
        bool aon  = id & AONCRG;
        int index = (id >> 16) & 0xFFFF;
        int bit   = 1 << (id & 0xFFFF);
        uintptr_t address;
        uintptr_t assert;
        uintptr_t status;

        id &= ~AONCRG;
        if (aon) {
            address = k_aon_crg_base;
            assert  = 0x38 + (4 * index);
            status  = 0x3c + (4 * index);
        } else {
            address = k_sys_crg_base;
            assert  = 0x2f8 + (4 * index);
            status  = 0x308 + (4 * index);
        }

        Reg32(address, assert) |= bit;

        while (Reg32(address, status) & bit)
            ;

        Reg32(address, assert) &= ~bit;

        while (!(Reg32(address, status) & bit))
            ;
    }
};

} // namespace QUARK
