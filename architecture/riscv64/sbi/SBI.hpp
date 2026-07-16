#pragma once

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/Modes.hpp>
#include <architecture/riscv64/PMP.hpp>
#include <architecture/riscv64/csrs.hpp>
#include <architecture/riscv64/sbi/IllegalInstruction.hpp>
#include <architecture/riscv64/sbi/LoadAccessFault.hpp>
#include <architecture/riscv64/sbi/LoadAddressMisaligned.hpp>
#include <architecture/riscv64/sbi/StoreAccessFault.hpp>
#include <architecture/riscv64/sbi/StoreAddressMisaligned.hpp>
#include <architecture/riscv64/sbi/Syscall.hpp>

namespace QUARK {

class SBI {
  public:
    static void init() {
        TrapHandler::install(sbi::Syscall::CODE, sbi::Syscall::dispatch, TrapHandler::Exception);
        TrapHandler::install(sbi::IllegalInstruction::CODE, sbi::IllegalInstruction::dispatch, TrapHandler::Exception);
        TrapHandler::install(sbi::LoadAccessFault::CODE, sbi::LoadAccessFault::dispatch, TrapHandler::Exception);
        TrapHandler::install(sbi::StoreAccessFault::CODE, sbi::StoreAccessFault::dispatch, TrapHandler::Exception);
        TrapHandler::install(sbi::LoadAddressMisaligned::CODE, sbi::LoadAddressMisaligned::dispatch, TrapHandler::Exception);
        TrapHandler::install(sbi::StoreAddressMisaligned::CODE, sbi::StoreAddressMisaligned::dispatch, TrapHandler::Exception);
    }
};

} // namespace QUARK
