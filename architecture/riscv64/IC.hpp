#pragma once

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/PLIC.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

class IC : Traits<PLIC> {
  using ExternalHandler = void (*)(size_t);

public:
  static void isr(ContextFrame *) {
    auto id = PLIC::claim();

    if (id != 0) {
      handlers_[id](id);
      PLIC::complete(id);
    }
  }

  static void install(size_t id, ExternalHandler handler) {
    assert(id < NumberOfInterruptions);
    handlers_[id] = handler;
    PLIC::priority(id, 1);
    PLIC::enable(id);
  }

  static void uninstall(size_t id) {
    assert(id < NumberOfInterruptions);
    handlers_[id] = nullptr;
    PLIC::priority(id, 0);
    PLIC::disable(id);
  }

private:
  static constinit inline ExternalHandler handlers_[NumberOfInterruptions];
};

} // namespace QUARK
  //
