#pragma once

#include <utility/meta/Array.hpp>
#include <utility/meta/IndexSequence.hpp>
#include <utility/meta/Pair.hpp>
#include <utility/meta/Tuple.hpp>

namespace QUARK::Meta {

struct Empty {};

template <bool B, typename True, typename False> struct IF {
  using Result = True;
};

template <typename True, typename False> struct IF<false, True, False> {
  using Result = False;
};

template <typename T, typename U> struct Same {
  static constexpr bool Result = false;
};

template <typename T> struct Same<T, T> {
  static constexpr bool Result = true;
};

template <typename T, typename U>
concept SameAs = Same<T, U>::Result;

template <typename... Tn> struct TypeList {
  static constexpr unsigned int Length = sizeof...(Tn);
};

template <typename List, unsigned int Index> struct GetFromTypeList;

template <typename Head, typename... Tail>
struct GetFromTypeList<TypeList<Head, Tail...>, 0> {
  using Result = Head;
};

template <typename Head, typename... Tail, unsigned int Index>
struct GetFromTypeList<TypeList<Head, Tail...>, Index> {
  using Result = typename GetFromTypeList<TypeList<Tail...>, Index - 1>::Result;
};

template <typename... Ts, typename Function>
void forEach(TypeList<Ts...>, Function f) {
  (f.template operator()<Ts>(), ...);
}

template <typename T>
concept Pointer = requires(T t) { []<typename U>(U *) {}(t); };

template <typename Base, typename Derived> struct IsBaseOf {
private:
  static char f(Base *);
  static int f(...);

public:
  static constexpr bool Result = sizeof(f((Derived *)nullptr)) == sizeof(char);
};

template <typename T> struct Remove {
  using Result = T;
};

template <typename T> struct Remove<const T> {
  using Result = T;
};

template <typename T> struct Remove<volatile T> {
  using Result = T;
};

template <typename T> struct Remove<const volatile T> {
  using Result = T;
};

template <typename T> struct Remove<T &> {
  using Result = T;
};

template <typename T> struct Remove<T &&> {
  using Result = T;
};

template <typename T, typename L> struct Execute {
  constexpr Execute(T &&t, L &&l) {
    using Removed = typename Remove<T>::Result;
    if constexpr (!Same<Removed, Meta::Empty>::Result) {
      l(t);
    }
  }
};

template <typename T, typename L> Execute(T &&, L &&) -> Execute<T, L>;

/* -------------------------------------------------------------------------
 */
/*                              Type Traits */
/* -------------------------------------------------------------------------
 */

template <typename T> struct IsInteger {
  static constexpr bool Result = false;
};
template <> struct IsInteger<char> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<signed char> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<unsigned char> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<short> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<unsigned short> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<int> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<unsigned int> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<long> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<unsigned long> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<long long> {
  static constexpr bool Result = true;
};
template <> struct IsInteger<unsigned long long> {
  static constexpr bool Result = true;
};
template <typename T> struct IsConst {
  static constexpr bool Result = false;
};
template <typename T> struct IsConst<const T> {
  static constexpr bool Result = true;
};
template <typename T> struct IsVoid {
  static constexpr bool Result = false;
};
template <> struct IsVoid<void> {
  static constexpr bool Result = true;
};

template <typename T>
concept Integer = IsInteger<T>::Result;

template <Integer T> struct IsSigned {
  static constexpr bool Result = T(-1) < T(0);
};

template <typename T>
concept Signed = IsSigned<T>::Result;

template <typename T>
concept Void = IsVoid<T>::Result;

template <typename T>
concept Const = IsConst<T>::Result;

} // namespace QUARK::Meta
