#ifndef __QUARK_UTILITY_META_MOVE__
#define __QUARK_UTILITY_META_MOVE__

#include <utility/meta/RemoveReference.hpp>

namespace QUARK::Meta {

template <typename T>
constexpr RemoveReference<T>::Result &&Move(T &&argument) {
  return static_cast<RemoveReference<T>::Result &&>(argument);
}

} // namespace QUARK::Meta

#endif
