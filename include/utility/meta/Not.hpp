#ifndef __QUARK_UTILITY_META_NOT__
#define __QUARK_UTILITY_META_NOT__

namespace QUARK::Meta {

template <template <typename> class Predicate> struct Not {
  template <typename T> struct Apply {
    static constexpr bool Result = !Predicate<T>::Result;
  };
};

} // namespace QUARK::Meta

#endif
