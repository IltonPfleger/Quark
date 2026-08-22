#ifndef __QUARK_UTILITY_META_IF__
#define __QUARK_UTILITY_META_IF__

namespace QUARK::Meta {

template <bool B, typename True, typename False> struct IF {
  using Result = True;
};

template <typename True, typename False> struct IF<false, True, False> {
  using Result = False;
};

} // namespace QUARK::Meta

#endif
