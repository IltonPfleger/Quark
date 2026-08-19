#ifndef __QUARK_RISCV64_IPI__
#define __QUARK_RISCV64_IPI__

#include <architecture/riscv64/CLINT.hpp>
#include <architecture/riscv64/ContextFrame.hpp>

namespace QUARK {

class IPI {
  using Handler = void (*)(void *);

public:
  struct Message {
    Handler handler;
    uintptr_t arguments[8];
  };

  static void isr(ContextFrame *) {
    messages_[mhartid()].handler(&messages_[mhartid()].arguments);
    CLINT::ipi(mhartid());
  }

  template <typename... Args>
  static void send(size_t hartid, Handler handler, Args &&...args) {
    static_assert(sizeof...(Args) <= 8);

    auto &message = messages_[hartid];

    message.handler = handler;

    size_t i = 0;
    ((message.arguments[i++] = reinterpret_cast<uintmax_t>(args)), ...);

    CPU::mb();
    CLINT::ipi(hartid);
  }

private:
  static inline Meta::Array<Traits<CPU>::Count, Message> messages_;
};

} // namespace QUARK

#endif
