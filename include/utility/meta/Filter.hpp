#ifndef __QUARK_UTILITY_META_FILTER__
#define __QUARK_UTILITY_META_FILTER__

#include <utility/meta/Concat.hpp>
#include <utility/meta/IF.hpp>
#include <utility/meta/Pack.hpp>

namespace QUARK::Meta {

template <template <typename> class Predicate, typename...> struct Filter;

template <template <typename> class Predicate> struct Filter<Predicate> {
  using Result = Pack<>;
};

template <template <typename> class Predicate, typename Head, typename... Tail>
struct Filter<Predicate, Head, Tail...> {
private:
  using Others = typename Filter<Predicate, Tail...>::Result;
  using Current =
      typename IF<Predicate<Head>::Result, Pack<Head>, Pack<>>::Result;

public:
  using Result = typename Concat<Current, Others>::Result;
};

} // namespace QUARK::Meta

#endif
