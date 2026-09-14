#pragma once

#include <architecture/ContextFrame.hpp>
#include <architecture/Modes.hpp>
#include <architecture/PMP.hpp>
#include <architecture/csrs.hpp>
#include <architecture/sbi/IllegalInstruction.hpp>
#include <architecture/sbi/LoadAccessFault.hpp>
#include <architecture/sbi/StoreAccessFault.hpp>
#include <architecture/sbi/Syscall.hpp>

namespace QUARK {

class SBI {
  using TrapHandler = QUARK::TrapHandler<MachineMode>;

public:
  static void init() {
    TrapHandler::install(sbi::Syscall::CODE, sbi::Syscall::dispatch,
                         TrapHandler::Exception);
    TrapHandler::install(sbi::IllegalInstruction::CODE,
                         sbi::IllegalInstruction::dispatch,
                         TrapHandler::Exception);
    TrapHandler::install(sbi::LoadAccessFault::CODE,
                         sbi::LoadAccessFault::dispatch,
                         TrapHandler::Exception);
    TrapHandler::install(sbi::StoreAccessFault::CODE,
                         sbi::StoreAccessFault::dispatch,
                         TrapHandler::Exception);
  }
};

} // namespace QUARK
