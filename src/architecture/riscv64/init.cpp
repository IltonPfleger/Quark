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
    static constexpr uint64_t MEDELEG = (1 << 0) |  // Instruction Address Misaligned
                                        (1 << 1) |  // Instruction Access Fault
                                        (1 << 2) |  // Illegal Instruction
                                        (1 << 3) |  // Breakpoint
                                        (1 << 4) |  // Load Address Misaligned
                                        (1 << 5) |  // Load Access Fault
                                        (1 << 6) |  // Store Address Misaligned
                                        (1 << 7) |  // Store Access Fault
                                        (1 << 8) |  // Ecall From U
                                        (1 << 12) | // Instruction Page Fault
                                        (1 << 13) | // Load Page Fault
                                        (1 << 15);  // Store Page Fault

    static constexpr uint64_t MIDELEG = (1 << 1) | // Supervisor Software Interrupt
                                        (1 << 5) | // Supervisor Timer Interrupt
                                        (1 << 9);  // Supervisor External Interrupt

    csrw<MachineMode::MEDELEG>(MEDELEG);
    csrw<MachineMode::MIDELEG>(MIDELEG);

    csrc<MachineMode::STATUS>(MachineMode::PP);
    csrc<MachineMode::STATUS>(SupervisorMode::PIRQE | SupervisorMode::IRQE);
    csrs<MachineMode::STATUS>(MachineMode::PP_S | MachineMode::PIRQE);

    csrw<SupervisorMode::SATP>(0);

    PMP::NAPOT<2>(0, 0, PMP::R | PMP::W | PMP::X);

    csrw<MachineMode::EPC>(__builtin_return_address(0));

    MachineMode::ret();
}

__attribute__((naked)) static void jvirtual() {
    csrs<SupervisorMode::STATUS>(SupervisorMode::PP_S);
    csrc<SupervisorMode::STATUS>(SupervisorMode::PIRQE | SupervisorMode::IRQE);
    csrw<SupervisorMode::EPC>(Memory::phys2virt(reinterpret_cast<uintptr_t>(__builtin_return_address(0))));
    SupervisorMode::ret();
}

__attribute__((naked)) static void supervisor() {
    CoreContext *context = CoreContextHandler<SupervisorMode>::init(CPU::tp());
    CoreContextHandler<SupervisorMode>::bind(context);
    SIC::init();
    init();
}

__attribute__((naked)) void epilogue() {
    CPU::tp(mhartid() - Traits<CPU>::Offset);

    CoreContextHandler<MachineMode>::bind(CoreContextHandler<MachineMode>::init(CPU::tp()));

    if constexpr (Traits<RISCV>::Hypervisor) {
        HIC::init();
    } else {
        MIC::init();
    }

    if constexpr (Traits<RISCV>::Supervisor) {
        sjump();

        if constexpr (Traits<Kernel>::Multitask) {
            if (CPU::tp() == Traits<CPU>::BSP) MMU::prologue();
            CPU::barrier();
            MMU::init();
            jvirtual();
            CPU::stack(Memory::phys2virt(CPU::stack()));
            CPU::barrier();
            if (CPU::tp() == Traits<CPU>::BSP) MMU::epilogue();
        }
        supervisor();
    }

    init();
}

extern "C" __attribute__((optimize("O0"), naked, used, section(".init"))) void prologue() {
    size_t core;
    uintptr_t position;

    // Disable Interruptions
    asm("csrw mie, zero");
    asm("csrc mstatus, 0x8");

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

    epilogue();
}

extern "C" __attribute__((optimize("O0"), naked, used, section(".init"))) void _init() { prologue(); }

} // namespace QUARK::riscv64
