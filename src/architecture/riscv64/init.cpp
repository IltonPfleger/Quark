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

extern "C" void init();

namespace QUARK::riscv64 {

__attribute__((naked)) static void sjump() {
    csrw<SupervisorMode::SATP>(0);
    PMP::NAPOT<2>(0, 0, PMP::R | PMP::W | PMP::X);
    csrw<MachineMode::MIDELEG>(0x1666);
    csrc<MachineMode::STATUS>(MachineMode::PP);
    csrc<MachineMode::STATUS>(SupervisorMode::PIRQE | SupervisorMode::IRQE);
    csrs<MachineMode::STATUS>(MachineMode::PP_S | MachineMode::PIRQE);
    csrw<MachineMode::EPC>(__builtin_return_address(0));
    MachineMode::ret();
}

__attribute__((naked)) static void jvirtual() {
    csrs<SupervisorMode::STATUS>(SupervisorMode::PP_S);
    csrc<SupervisorMode::STATUS>(SupervisorMode::PIRQE | SupervisorMode::IRQE);
    csrw<SupervisorMode::EPC>(Memory::phys2virt(reinterpret_cast<uintptr_t>(__builtin_return_address(0))));
    SupervisorMode::ret();
}

extern "C" __attribute__((optimize("O0"), naked, used, section(".init"))) void _init() {
    size_t core;
    uintptr_t position;

    // Disable Interruptions
    asm("csrc mstatus, 0x8");

    // Save Return Address
    asm("csrw mscratch, ra");

    // Found Which Core It's Running
    asm("csrr %0, mhartid" : "=r"(core));

    if (core < Traits<CPU>::Offset) {
        asm("wfi");
    }

    core -= Traits<CPU>::Offset;

    if (core >= Traits<CPU>::Active) {
        asm("wfi");
    }

    // Get The Code Position For Position Independent Environments
    asm("auipc %0, 0" : "=r"(position));
    position &= ~((1ULL << 30) - 1);

    // Get The Boot Stack
    asm("mv sp, %0" ::"r"(position + Traits<Memory>::Size - (Traits<Memory>::StackSize * core)));

    // Setup System Boot Info
    if (core == Traits<CPU>::BSP) {
        new (&__amm) decltype(__amm)(position, Traits<Memory>::Size);
        new (&__bmm) decltype(__bmm)(Traits<MemoryMap>::RamEnd, Traits<Memory>::StackSize * Traits<CPU>::Active);
    }

    CPU::barrier();

    csrw<MachineMode::IE>(0);

    TrapHandler::init();

    CoreContextHandler<MachineMode>::bind(CoreContextHandler<MachineMode>::init(core));

    if constexpr (Traits<RISCV>::Hypervisor) {
        HIC::init();
    } else {
        MIC::init();
    }

    if constexpr (Traits<RISCV>::Supervisor) {
        sjump();

        if constexpr (Traits<Kernel>::Multitask) {
            if (core == Traits<CPU>::BSP) MMU::prologue();
            CPU::barrier();
            MMU::init();
            jvirtual();
            CPU::sp(Memory::phys2virt(CPU::sp()));
            CPU::barrier();
            if (core == Traits<CPU>::BSP) MMU::epilogue();
        }

        CoreContext *context = CoreContextHandler<SupervisorMode>::init(core);
        CoreContextHandler<SupervisorMode>::bind(context);

        SIC::init();
    }

    init();
}

} // namespace QUARK::riscv64
