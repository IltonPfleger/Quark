#ifndef __QUARK_RISCV64_IPI__
#define __QUARK_RISCV64_IPI__

#include <architecture/CLINT.hpp>
#include <architecture/ContextFrame.hpp>
#include <synchronization/Mutex.hpp>

namespace QUARK {

class IPI {
public:
  using Handler = void (*)(void *);

  struct Message {
    Mutex lock{};
    Handler handler{nullptr};
    uintptr_t arguments[8];
    bool pending{false};
  };

  template <typename... Args>
  static void send(size_t hartid, Handler handler, Args &&...args) {
    static_assert(sizeof...(Args) <= 8);
    assert(hartid <= Traits<CPU>::Count);

    Message &message = channel(hartid);

    message.lock.acquire();

    message.pending = true;

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

    Message &message = channel(hartid);

    if (message.pending) {
      message.pending = false;
      message.handler(&message.arguments);
      message.lock.release();
    }
  }

private:
  static Message &channel(size_t i = mhartid()) {
    static constinit Meta::Array<Traits<CPU>::Count, Message> channels_{};
    return channels_[i];
  }
};

// inline Meta::Array<Traits<CPU>::Count, IPI::Message> IPI::channels_{};

} // namespace QUARK

#endif
