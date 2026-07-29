#ifndef __QUARK_UTILITY_META_PAIR__
#define __QUARK_UTILITY_META_PAIR__

namespace QUARK::Meta {

template <typename First, typename Second> class Pair {
  public:
    constexpr Pair() = default;

    constexpr Pair(const First &first, const Second &second)
        : first(first),
          second(second) {}

    First first;
    Second second;
};

template <typename First, typename Second> Pair(const First &, const Second &) -> Pair<First, Second>;

} // namespace QUARK::Meta

#endif
