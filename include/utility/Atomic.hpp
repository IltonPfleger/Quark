#ifndef __QUARK_ATOMIC__
#define __QUARK_ATOMIC__

#include <Meta.hpp>
#include <architecture/CPU.hpp>

namespace QUARK {

template <typename T> class Atomic {
  public:
    Atomic(T value = 0)
        : value_(value) {}

    T load() const { return CPU::Atomic::load(value_); }

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

    bool tsl() { return CPU::Atomic::tsl(value_); }

    bool cas(T &expected, T desired) { return CPU::Atomic::cas(value_, expected, desired); }

    T exchange(T desired) { return CPU::Atomic::exchange(value_, desired); }

    void store(T value) { CPU::Atomic::store(value_, value); }

    T operator++()
        requires Meta::Integer<T>
    {
        return finc();
    }

    T operator--()
        requires Meta::Integer<T>
    {
        return fdec();
    }

    T operator|=(T mask)
        requires Meta::Integer<T>
    {
        T current = load();
        while (true) {
            T desired = current | mask;
            if (cas(current, desired)) return desired;
        }
    }

    T operator&=(T mask)
        requires Meta::Integer<T>
    {
        T current = load();
        while (true) {
            T desired = current & mask;
            if (cas(current, desired)) return desired;
        }
    }

    operator T() const { return load(); }

  private:
    T value_;
};

} // namespace QUARK

#endif
