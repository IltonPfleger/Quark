#ifndef __QUARK_RISCV64_TRAP_HANDLER_HEADER__
#define __QUARK_RISCV64_TRAP_HANDLER_HEADER__

#include <architecture/riscv64/ExceptionHandler.hpp>

namespace QUARK {

class TrapHandler {
  public:
    enum Type { Exception = 0, Interrupt = 1 };

  private:
    using ID      = size_t;
    using Index   = uintmax_t;
    using Handler = void (*)(ContextFrame *);

    static Index irq2index(ID id, Type type) { return id + (type * NumberOfExceptions); }

    static void dispatch(ContextFrame *c) {
        uintmax_t cause = c->cause;
        Type type       = (Type)(cause >> 63);
        ID id           = cause & ~(1ULL << 63);
        Index index     = irq2index(id, type);

        assert(index < NumberOfHandlers, index);
        assert(s_handlers[index], index, " ", id);

        s_handlers[index](c);
    }

    template <typename Privilege, bool ChangeStack>
    __attribute__((naked, optimize("O0"), aligned(4))) static void entry() {
        using Context = ContextTemplate<Privilege, ChangeStack>;
        dispatch(Context::push());
        Context::pop();
    };

  public:
    static void install(ID id, Handler handler, Type type = Interrupt) {
        Index index = irq2index(id, type);
        assert(index < NumberOfHandlers, index);
        s_handlers[index] = handler;
    };

    template <typename Privilege, bool ChangeStack> static void init() {
        csrw<Privilege::TVEC>(entry<Privilege, ChangeStack>);
    }

    static void init() {
        for (int i = 0; i < 16; i++)
            install(i, ExceptionHandler::onTrap, Exception);
    }

  private:
    static constexpr int NumberOfExceptions    = 16;
    static constexpr int NumberOfInterruptions = 16;
    static constexpr int NumberOfHandlers      = NumberOfExceptions + NumberOfInterruptions;
    static constinit inline Handler s_handlers[NumberOfHandlers];
};

} // namespace QUARK

#endif
