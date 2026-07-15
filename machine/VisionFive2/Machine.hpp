#pragma once

#include <Traits.hpp>
#include <architecture/riscv64/init.hpp>
#include <drivers/cache/SiFiveU74_L2_CacheController.hpp>
#include <drivers/clock/JH7110_Clock_Controller.hpp>
#include <drivers/dvfs/JH7110_DVFS_Controller.hpp>
#include <machine/VisionFive2/GPIO.hpp>

namespace QUARK {

class Machine : Driver {
    using Clock_Controller = JH7110_Clock_Controller<void>;

  public:
    static void init() {
        riscv64::init();

        if (CPU::id() == Traits<CPU>::BSP) {
            JH7110_DVFS_Controller dvfs;

            Clock_Controller::divide(Clock_Controller::SYSCRG_CLK_CPU_CORE, 2);
            Clock_Controller::multiplex(Clock_Controller::SYSCRG_CLK_CPU_ROOT, 0);
            Timer::Delay(1'000);

            PLL0::rate(1500000000);
            Timer::Delay(1'000);

            Clock_Controller::multiplex(Clock_Controller::SYSCRG_CLK_CPU_ROOT, 1);
            Timer::Delay(1'000);

            dvfs.set(dvfs.available()[dvfs.available().length() - 1]);
        }

        CPU::barrier();

        if (CPU::id() == Traits<CPU>::BSP) {
            /* ---***--- GMAC0 ---***--- */
            Clock_Controller::enable(Clock_Controller::SYSCRG_CLK_GMAC_PHY);
            Clock_Controller::enable(Clock_Controller::SYSCRG_CLK_GMAC0_GTX);
            Clock_Controller::enable(Clock_Controller::SYSCRG_CLK_GMAC_SOURCE);
            Clock_Controller::enable(Clock_Controller::SYSCRG_CLK_GMAC5_AXI64_AHB);
            Clock_Controller::enable(Clock_Controller::SYSCRG_CLK_GMAC5_AXI64_AXI);
            Clock_Controller::reset(Clock_Controller::SYSCRG_CLK_RSTN_U1_GMAC5_AXI64_ARESETN_I);
            Clock_Controller::reset(Clock_Controller::SYSCRG_CLK_RSTN_U1_GMAC5_AXI64_HRESET_N);
            Clock_Controller::divide(Clock_Controller::AONCRG_CLK_GMAC0_RMII_RTX, 30);
            Clock_Controller::enable(Clock_Controller::AONCRG_CLK_GMAC0_TX);
            Clock_Controller::invert(Clock_Controller::AONCRG_CLK_GMAC5_AXI64_TX_INVERTER, true);
            Clock_Controller::multiplex(Clock_Controller::AONCRG_CLK_GMAC0_TX, 1);
            Clock_Controller::enable(Clock_Controller::AONCRG_CLK_GMAC0_AHB);
            Clock_Controller::enable(Clock_Controller::AONCRG_CLK_GMAC0_AXI);
            Clock_Controller::reset(Clock_Controller::AONCRG_CLK_RSTN_GMAC5_AXI64_AXI);
            Clock_Controller::reset(Clock_Controller::AONCRG_CLK_RSTN_GMAC5_AXI64_AHB);
            /* ---***--- CAN0 ---***--- */
            Clock_Controller::divide(Clock_Controller::SYSCRG_CLK_CAN0_CTRL_CORE, 15);
            Clock_Controller::enable(Clock_Controller::SYSCRG_CLK_CAN0_CTRL_APB);
            Clock_Controller::enable(Clock_Controller::SYSCRG_CLK_CAN0_CTRL_TIMER);
            Clock_Controller::enable(Clock_Controller::SYSCRG_CLK_CAN0_CTRL_CORE);
            Clock_Controller::reset(Clock_Controller::SYSCRG_CLK_RSTN_U0_CAN_CTRL_APB);
            Clock_Controller::reset(Clock_Controller::SYSCRG_CLK_RSTN_U0_CAN_CTRL_CORE);
            Clock_Controller::reset(Clock_Controller::SYSCRG_CLK_RSTN_U0_CAN_CTRL_TIMER);
            GPIO::map(GPIO::OutputSignal::GPO_SYS_IOMUX_U0_CAN_CTRL_TXD, 42);
            GPIO::map(GPIO::InputSignal::GPI_SYS_IOMUX_U0_CAN_CTRL_RXD, 43);
            GPIO::map(GPIO::OutputSignal::GPO_SYS_IOMUX_U0_CAN_CTRL_STB, 47);
        }

        Meta::forEach(Traits<CacheController>::Devices{}, []<typename T>() { T::init(); });
        Meta::forEach(Traits<UART>::Devices{}, []<typename T>() { T::init(); });
        CPU::barrier();
    }

    static void shutdown() { CPU::halt(); }
};

} // namespace QUARK

// #include <drivers/can/IPMSCANFD.hpp>
#include <drivers/ethernet/DWC_Ether_QoS.hpp>
#include <drivers/uart/UART16550.hpp>
