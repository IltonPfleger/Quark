#pragma once

#include <BootInformation.hpp>
#include <Traits.hpp>
#include <architecture/common/CPU.hpp>
#include <architecture/riscv64/Context.hpp>
#include <architecture/riscv64/CoreContextHandler.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/csrs.hpp>
#include <memory/operators.hpp>
#include <utility/Endian.hpp>
#include <utility/Guard.hpp>

namespace QUARK {

class CPU : public ArchitectureCommon::CPU {
  public:
    static constexpr bool Supervisor  = Traits<RISCV>::Supervisor;
    static constexpr bool Virtualized = Traits<Payload>::Virtualized;

    using NotSupervisorContext = Meta::IF<!Virtualized, QUARK::MachineContext<>, HypervisorContext>::Result;
    using Context              = Meta::IF<Supervisor, SupervisorContext<>, NotSupervisorContext>::Result;

    static void halt() { asm("1: wfi; j 1b"); }
    static auto idle() { asm("wfi"); }
    static void syscall() { asm("ecall"); }
    static void mb() { asm("fence iorw, iorw" ::: "memory"); }
    static constexpr uint32_t hi32(uint64_t v) { return static_cast<uint32_t>(v >> 32); }
    static constexpr uint32_t lo32(uint64_t v) { return static_cast<uint32_t>(v); }
    static constexpr uint64_t htobe64(uint64_t x) { return Endian::le2be64(x); }
    static constexpr uint32_t htobe32(uint32_t x) { return Endian::le2be32(x); }
    static constexpr uint16_t htobe16(uint16_t x) { return Endian::le2be16(x); }
    static constexpr uint64_t be64toh(uint64_t x) { return htobe64(x); }
    static constexpr uint32_t be32toh(uint32_t x) { return htobe32(x); }
    static constexpr uint16_t be16toh(uint16_t x) { return htobe16(x); }

    template <bool M = !Supervisor> static uint32_t id() {
        if constexpr (M) return csrr<MachineMode::HARTID>() - Traits<CPU>::Offset;
        return CoreContextHandler<SupervisorMode>::cpu();
    }

    class IRQ {
      public:
        static void enable(uint64_t status = KernelMode::IRQE) {
            mb();
            csrs<KernelMode::STATUS>(status);
        }

        static uint64_t disable() {
            uint64_t status = csrrc<KernelMode::STATUS>(KernelMode::IRQE);
            mb();
            return status & KernelMode::IRQE;
        }

        using Guard = QUARK::Guard<void, disable, enable>;
    };

    __attribute__((naked, optimize("O0"))) static void init() {
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

        barrier();

        // Restore Return Address
        asm("csrr ra, mscratch");

        // Return
        asm("ret");
    }
};

} // namespace QUARK
