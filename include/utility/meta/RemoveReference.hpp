#ifndef __QUARK_UTILITY_META_REMOVE_REFERENCE__
#define __QUARK_UTILITY_META_REMOVE_REFERENCE__

namespace QUARK::Meta {

template <typename T> struct RemoveReference {
  using Result = T;
};

template <typename T> struct RemoveReference<T &> {
  using Result = T;
};

template <typename T> struct RemoveReference<T &&> {
  using Result = T;
};

} // namespace QUARK::Meta

#endif
