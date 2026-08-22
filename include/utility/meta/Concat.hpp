#ifndef __QUARK_UTILITY_META_CONCAT__
#define __QUARK_UTILITY_META_CONCAT__

#include <utility/meta/Pack.hpp>

namespace QUARK::Meta {

template <typename T, typename U> struct Concat;

template <typename... T, typename... U> struct Concat<Pack<T...>, Pack<U...>> {
  using Result = Pack<T..., U...>;
};

} // namespace QUARK::Meta

#endif
