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
  Node(T &&t, U &&u)
      : value(static_cast<V &&>(t)), criterion(static_cast<C &&>(u)),
        next(nullptr) {}

  template <typename T>
    requires(!Meta::IsVoid<V>::Result)
  Node(T &&t) : value(static_cast<V &&>(t)), criterion(), next(nullptr) {}

  template <typename T>
    requires Meta::IsVoid<V>::Result
  Node(T &&t) : value(), criterion(static_cast<T &&>(t)), next(nullptr) {}

  Node() : value(), criterion(), next(nullptr) {}

public:
  [[no_unique_address]] Value value;
  [[no_unique_address]] Criterion criterion;
  [[no_unique_address]] Previous previous;
  Next next;
};

} // namespace collections

} // namespace QUARK
