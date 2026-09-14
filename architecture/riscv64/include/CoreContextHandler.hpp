#ifndef __RISCV64_CORE_CONTEXT_HANDLER_HEADER__
#define __RISCV64_CORE_CONTEXT_HANDLER_HEADER__

#include <Traits.hpp>
#include <architecture/CoreContext.hpp>
#include <architecture/Modes.hpp>
#include <architecture/csrs.hpp>

namespace QUARK {

template <typename T> class CoreContextHandler {

public:
  static CoreContext *init(size_t id) {
    contexts_[id].core = id;
    return &contexts_[id];
  }

  static void bind(CoreContext *source) { csrw<T::SCRATCH>(source); }
  static CoreContext *current() {
    return reinterpret_cast<CoreContext *>(csrr<T::SCRATCH>());
  }
  static void stack(uintptr_t sp) { current()->ksp = sp; }
  static size_t cpu() { return current()->core; }

private:
  static inline CoreContext contexts_[Traits<CPU>::Active];
};

} // namespace QUARK

#endif
