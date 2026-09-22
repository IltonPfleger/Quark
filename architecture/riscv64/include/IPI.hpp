#ifndef __QUARK_RISCV64_IPI__
#define __QUARK_RISCV64_IPI__

#include <Mutex.hpp>
#include <architecture/CLINT.hpp>
#include <architecture/ContextFrame.hpp>

namespace QUARK {

class IPI {
public:
  using Handler = void (*)(void *);

  struct Message {
    Handler handler{nullptr};
    Mutex lock{};
    uintptr_t arguments[8]{};
  };

  template <typename... Args>
  static void send(size_t hartid, Handler handler, Args &&...args) {
    static_assert(sizeof...(Args) <= 8);
    assert(hartid <= Traits<CPU>::Count);

    auto &message = IPI::channels_[hartid];

    message.lock.acquire();

    message.handler = handler;

    size_t i = 0;
    ((message.arguments[i++] = reinterpret_cast<uintptr_t>(args)), ...);

    CPU::mbw();
    CLINT::ipi(hartid, 1);
  }

  static void isr(ContextFrame *) {
    size_t hartid = mhartid();
    CLINT::ipi(hartid, 0);
    CPU::mbr();
    channels_[hartid].handler(&channels_[hartid].arguments);
    channels_[hartid].lock.release();
  }

private:
  static Meta::Array<Traits<CPU>::Count, Message> channels_;
};

inline Meta::Array<Traits<CPU>::Count, IPI::Message> IPI::channels_{};

} // namespace QUARK

#endif
