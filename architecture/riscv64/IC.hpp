#pragma once

#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/PLIC.hpp>
#include <utility/Debug.hpp>

namespace QUARK {

class IC : Traits<PLIC> {
    using ID              = size_t;
    using ExternalHandler = void (*)(ID);

  public:
    static void onTrap(ContextFrame *) {
        auto id = PLIC::claim();
        if (id != 0) {
            handlers_[id](id);
            PLIC::complete(id);
        }
    }

    static void install(ID id, ExternalHandler handler) {
        assert(id < NumberOfInterruptions);
        handlers_[id] = handler;
        PLIC::priority(id, 1);
        PLIC::enable(id);
    }

  private:
    static constinit inline ExternalHandler handlers_[NumberOfInterruptions];
};

} // namespace QUARK
