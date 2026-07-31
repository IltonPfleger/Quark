#ifndef __QUARK_UTILITY_META_INDEX_SEQUENCE__
#define __QUARK_UTILITY_META_INDEX_SEQUENCE__

namespace QUARK::Meta {

template <unsigned long... Is> struct IndexSequence {
    using Result = IndexSequence;
};

template <unsigned long N, unsigned long... Is> struct MakeIndexSequenceHelper : MakeIndexSequenceHelper<N - 1, N - 1, Is...> {};

template <unsigned long... Is> struct MakeIndexSequenceHelper<0, Is...> {
    using Result = IndexSequence<Is...>;
};

template <unsigned long N> using MakeIndexSequence = MakeIndexSequenceHelper<N>::Result;

} // namespace QUARK::Meta

#endif
