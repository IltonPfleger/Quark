#ifndef __QUARK_RISCV64_IPI__
#define __QUARK_RISCV64_IPI__

#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/ContextFrame.hpp>
#include <architecture/riscv64/VirtualCPU.hpp>

namespace QUARK {

class IPI {
public:
  struct Message {
    using Handler = void (*)(void *);
    Handler function;
    uintptr_t arguments[8];
  };

  static void isr(ContextFrame *) {
    VirtualCPU::onInterProcessorInterrupt();
    messages_[mhartid()].function(&messages_[mhartid()].arguments);
    CLINT::ipi(mhartid());
  }

  static void send(size_t hartid, const Message &&message) {
    messages_[hartid] = message;
    CLINT::ipi(hartid);
  }

private:
  static inline Meta::Array<Traits<CPU>::Count, Message> messages_;
};

} // namespace QUARK

#endif
