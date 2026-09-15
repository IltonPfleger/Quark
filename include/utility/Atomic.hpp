#ifndef __QUARK_ATOMIC__
#define __QUARK_ATOMIC__

#include <Meta.hpp>
#include <architecture/CPU.hpp>

namespace QUARK {

template <typename T> class Atomic {
public:
  constexpr Atomic(T value = 0) : value_(value) {}

  T finc()
    requires Meta::Integer<T>
  {
    return CPU::Atomic::finc(value_);
  }

  T fdec()
    requires Meta::Integer<T>
  {
    return CPU::Atomic::fdec(value_);
  }

  T fand(T mask) {
    while (true) {
      T current = load();
      T desired = current & mask;
      if (cas(current, desired))
        return current;
    }
  }

  T fior(T mask) {
    while (true) {
      T current = load();
      T desired = current | mask;
      if (cas(current, desired))
        return current;
    }
  }

  bool tsl() { return CPU::Atomic::tsl(value_); }

  bool cas(T &expected, T desired) {
    return CPU::Atomic::cas(value_, expected, desired);
  }

  void store(T value) { CPU::Atomic::store(value_, value); }

  T load() const { return CPU::Atomic::load(value_); }

  T operator++() { return finc(); }

  T operator--() { return fdec(); }

  T operator|=(T mask) { return fior(mask) | mask; }

  T operator&=(T mask) { return fand(mask) & mask; }

  operator T() const { return load(); }

private:
  T value_;
};

} // namespace QUARK

#endif
