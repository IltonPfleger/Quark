#pragma once

#include <Meta.hpp>

namespace QUARK {

namespace collections {

template <typename V = void, typename C = void, bool P = false> struct Node {
  using Value =
      typename Meta::IF<Meta::IsVoid<V>::Result, Meta::Empty, V>::Result;
  using Criterion =
      typename Meta::IF<Meta::IsVoid<C>::Result, Meta::Empty, C>::Result;
  using Next = Node *;
  using Previous = typename Meta::IF<!P, Meta::Empty, Next>::Result;

  template <typename T, typename U>
    requires(!Meta::IsVoid<V>::Result)
  constexpr Node(T &&t, U &&u)
      : value(static_cast<V &&>(t)), criterion(static_cast<C &&>(u)),
        previous(), next(nullptr) {}

  template <typename T>
    requires(!Meta::IsVoid<V>::Result)
  constexpr Node(T &&t)
      : value(static_cast<V &&>(t)), criterion(), previous(), next(nullptr) {}

  template <typename T>
    requires Meta::IsVoid<V>::Result
  constexpr Node(T &&t)
      : value(), criterion(static_cast<T &&>(t)), previous(), next(nullptr) {}

  constexpr Node() : value(), criterion(), previous(), next(nullptr) {}

public:
  [[no_unique_address]] Value value{};
  [[no_unique_address]] Criterion criterion{};
  [[no_unique_address]] Previous previous{};
  Next next{nullptr};
};

} // namespace collections

} // namespace QUARK
