#ifndef __QUARK_UTILITY_META_PACK__
#define __QUARK_UTILITY_META_PACK__

namespace QUARK {

namespace Meta {

template <typename...> struct Pack {};

template <typename List, unsigned int Index> struct PackIndex;

template <typename Head, typename... Tail>
struct PackIndex<Pack<Head, Tail...>, 0> {
  using Result = Head;
};

template <typename Head, typename... Tail, unsigned int Index>
struct PackIndex<Pack<Head, Tail...>, Index> {
  using Result = typename PackIndex<Pack<Tail...>, Index - 1>::Result;
};

} // namespace Meta

} // namespace QUARK

#endif
