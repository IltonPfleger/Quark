#ifndef __QUARK_UTILITY_META_TUPLE__
#define __QUARK_UTILITY_META_TUPLE__

#include <utility/meta/Pack.hpp>

namespace QUARK::Meta {

template <typename...> struct Tuple;

template <> struct Tuple<> {
  template <typename... Args> constexpr Tuple(Args &&...) {}
};

template <typename Head, typename... Tail> struct Tuple<Head, Tail...> {
  Head value_;
  Tuple<Tail...> next_;

  template <typename... Args>
  constexpr Tuple(Args &&...args) : value_(args...), next_(args...) {}
};

template <typename... Ts> struct Tuple<Pack<Ts...>> : Tuple<Ts...> {
  template <typename... Args>
  constexpr Tuple(Args &&...args) : Tuple<Ts...>(args...) {}
};

template <typename Function> void forEach(Tuple<> &, Function) {}

template <typename Head, typename... Tail, typename Function>
void forEach(Tuple<Head, Tail...> &tuple, Function f) {
  f(tuple.value_);
  forEach(tuple.next_, f);
}

} // namespace QUARK::Meta

#endif
