#ifndef __QUARK_RISCV64_TRAP_HANDLER_HEADER__
#define __QUARK_RISCV64_TRAP_HANDLER_HEADER__

#include <architecture/riscv64/ExceptionHandler.hpp>

namespace QUARK {

template <typename Privilege> class TrapHandler {
public:
  enum Type { Exception = 0, Interrupt = 1 };

private:
  using Handler = void (*)(ContextFrame *);

  static size_t irq2index(size_t id, Type type) {
    return id + (type * NumberOfExceptions);
  }

  static void dispatch(ContextFrame *c) {
    uintmax_t cause = c->cause;

    Type type = (Type)(cause >> 63);
    size_t id = cause & ~(1ULL << 63);

    size_t index = irq2index(id, type);

    assert(index < NumberOfHandlers, index);
    assert(handlers_[index], index, " ", id);

    handlers_[index](c);
  }

  template <bool ChangeStack>
  __attribute__((naked, optimize("O0"), aligned(4))) static void entry() {
    using Context = ContextTemplate<Privilege, ChangeStack>;
    dispatch(Context::push());
    Context::pop();
  };

public:
  static void install(size_t id, Handler handler, Type type = Interrupt) {
    size_t index = irq2index(id, type);
    assert(index < NumberOfHandlers, index);
    handlers_[index] = handler;
  };

  template <bool ChangeStack> static void init() {
    for (int i = 0; i < NumberOfExceptions; i++) {
      install(i, ExceptionHandler::esr, Exception);
    }
    csrw<Privilege::TVEC>(entry<ChangeStack>);
  }

public:
  static constexpr int NumberOfExceptions = 16;
  static constexpr int NumberOfInterruptions = 16;

private:
  static constexpr int NumberOfHandlers =
      NumberOfExceptions + NumberOfInterruptions;
  static constinit inline Handler handlers_[NumberOfHandlers];
};

} // namespace QUARK

#endif
