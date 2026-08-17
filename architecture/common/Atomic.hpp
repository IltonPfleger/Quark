#pragma once

#include <Meta.hpp>

namespace QUARK::ArchitectureCommon {

class Atomic {
public:
  static auto tsl(auto &v) {
    return __atomic_test_and_set(&v, __ATOMIC_SEQ_CST);
  }
  static auto fdec(auto &v, unsigned int x = 1) {
    return __atomic_fetch_sub(&v, x, __ATOMIC_SEQ_CST);
  }
  static auto finc(auto &v, unsigned int x = 1) {
    return __atomic_fetch_add(&v, x, __ATOMIC_SEQ_CST);
  }
  static auto store(auto &v, auto x) {
    return __atomic_store_n(&v, x, __ATOMIC_RELEASE);
  }

  template <typename T> static auto load(const T &reference) {
    typename Meta::Remove<T>::Result result;
    __atomic_load(&reference, &result, __ATOMIC_CONSUME);
    return result;
  }

  template <typename T>
  static bool cas(T &value, Meta::Remove<T>::Result expected,
                  Meta::Remove<T>::Result desired) {
    return __atomic_compare_exchange_n(&value, &expected, desired, false,
                                       __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
  }

  template <typename T>
  static Meta::Remove<T>::Result exchange(T &value,
                                          Meta::Remove<T>::Result desired) {
    return __atomic_exchange_n(&value, desired, __ATOMIC_ACQ_REL);
  }
};

} // namespace QUARK::ArchitectureCommon
