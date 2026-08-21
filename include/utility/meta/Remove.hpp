#ifndef __QUARK_UTILITY_META_REMOVE__
#define __QUARK_UTILITY_META_REMOVE__

namespace QUARK::Meta {

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

} // namespace QUARK::Meta

#endif
