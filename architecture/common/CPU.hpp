#pragma once

#include <Traits.hpp>
#include <architecture/common/Atomic.hpp>

namespace QUARK {

namespace ArchitectureCommon {

class CPU {
public:
  using Atomic = QUARK::ArchitectureCommon::Atomic;

  static void barrier() {
    static constinit volatile bool gsense = 1;
    static constinit volatile int ready = Traits<QUARK::CPU>::Active;

    auto sense = !Atomic::load(gsense);
    int position = Atomic::fdec(ready);

    if (position == 1) {
      Atomic::store(ready, Traits<QUARK::CPU>::Active);
      Atomic::store(gsense, sense);
    } else {
      while (Atomic::load(gsense) != sense)
        ;
    }
  }
};

} // namespace ArchitectureCommon

} // namespace QUARK
