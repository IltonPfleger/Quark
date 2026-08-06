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
    static constexpr bool Supervisor     = Traits<RISCV>::Supervisor;
    static constexpr bool Virtualization = Traits<Payload>::Virtualization;

    using NotSupervisorContext = Meta::IF<!Virtualization, QUARK::MachineContext<>, HypervisorContext>::Result;
    using Context              = Meta::IF<Supervisor, SupervisorContext<>, NotSupervisorContext>::Result;

    __attribute__((naked)) static uintptr_t sp() { asm("mv a0, sp; ret"); }
    __attribute__((naked)) static void sp(uintptr_t) { asm("mv sp, a0; ret"); }
    __attribute__((naked)) static uintptr_t tp() { asm("mv a0, tp; ret"); }
    __attribute__((naked)) static void tp(uintptr_t) { asm("mv tp, a0; ret"); }

    __attribute__((always_inline)) static void syscall() { asm("ecall"); }

    static void halt() { asm("1: wfi; j 1b"); }

    static auto idle() { asm("wfi"); }

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
            csrs<KernelMode::STATUS>(status & KernelMode::IRQE);
        }

        static uint64_t disable() {
            uint64_t status = csrrc<KernelMode::STATUS>(KernelMode::IRQE);
            mb();
            return status & KernelMode::IRQE;
        }

        using Guard = QUARK::Guard<void, disable, enable>;
    };
};

} // namespace QUARK
