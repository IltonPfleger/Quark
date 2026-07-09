#pragma once

#include <abi/ABI.hpp>
#include <types.hpp>

namespace QUARK::ABI {

class Handler {
  public:
    using Arguments = uintmax_t *;
    static void *dispatch(Function, const Arguments);
};

} // namespace QUARK::ABI
