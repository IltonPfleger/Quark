#pragma once

#include <architecture/riscv64/CPU.hpp>
#include <architecture/riscv64/CoreContextHandler.hpp>
#include <architecture/riscv64/HIC.hpp>
#include <architecture/riscv64/IC.hpp>
#include <architecture/riscv64/MIC.hpp>
#include <architecture/riscv64/MMU.hpp>
#include <architecture/riscv64/PLIC.hpp>
#include <architecture/riscv64/PMP.hpp>
#include <architecture/riscv64/SIC.hpp>

#include <architecture/riscv64/TrapHandler.hpp>

namespace QUARK {

namespace riscv64 {

__attribute__((naked)) static void supervisor() {
    csrw<SupervisorMode::SATP>(0);
    PMP::NAPOT<2>(0, 0, PMP::R | PMP::W | PMP::X);
    csrw<MachineMode::MIDELEG>(0x1666);
    csrs<MachineMode::STATUS>(MachineMode::PP_S | MachineMode::PIRQE);
    csrc<MachineMode::STATUS>(SupervisorMode::PIRQE | SupervisorMode::IRQE);
    csrw<MachineMode::EPC>(__builtin_return_address(0));
    MachineMode::ret();
}

inline void init() {
    csrw<MachineMode::IE>(0);

    TrapHandler::init();

    size_t core = CPU::id<true>();

    CoreContextHandler<MachineMode>::bind(CoreContextHandler<MachineMode>::init(core));

    if constexpr (Traits<RISCV>::Hypervisor) {
        HIC::init();
    } else {
        MIC::init();
    }

    if constexpr (Traits<RISCV>::Supervisor) {
        CoreContext *context = CoreContextHandler<SupervisorMode>::init(core);
        supervisor();
        CoreContextHandler<SupervisorMode>::bind(context);
        SIC::init();
    }

    if constexpr (Traits<Kernel>::Multitask) MMU::init();
}

} // namespace riscv64

} // namespace QUARK
