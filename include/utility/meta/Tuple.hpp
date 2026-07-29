#ifndef __QUARK_UTILITY_META_TUPLE__
#define __QUARK_UTILITY_META_TUPLE__

namespace QUARK::Meta {

template <typename...> struct Tuple;

template <> struct Tuple<> {};

template <typename Head, typename... Tail> struct Tuple<Head, Tail...> {
    Head m_value;
    Tuple<Tail...> m_next;
};

template <typename Function> void forEach(Tuple<> &, Function) {}

template <typename Head, typename... Tail, typename Function> void forEach(Tuple<Head, Tail...> &tuple, Function f) {
    f(tuple.m_value);
    forEach(tuple.m_next, f);
}

} // namespace QUARK::Meta

#endif
