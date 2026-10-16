#pragma once

#include <Traits.hpp>

namespace QUARK {

namespace Architecture {

class Timer {
protected:
  static void handler(size_t channel);
};

} // namespace Architecture

} // namespace QUARK
