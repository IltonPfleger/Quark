#ifndef __QUARK_UTILITY_META_ARRAY__
#define __QUARK_UTILITY_META_ARRAY__

namespace QUARK::Meta {

template <unsigned N, typename T> struct Array {
    constexpr T &operator[](unsigned i) { return data[i]; }
    constexpr const T &operator[](unsigned i) const { return data[i]; }

    constexpr T *begin() { return data; }
    constexpr T *end() { return data + N; }

    constexpr const T *begin() const { return data; }
    constexpr const T *end() const { return data + N; }
    constexpr unsigned length() const { return N; }

    T data[N];
};

template <typename T> struct Array<0, T> {
    constexpr T *begin() { return nullptr; }
    constexpr T *end() { return nullptr; }
    constexpr T &operator[](unsigned) { __builtin_unreachable(); }
    constexpr const T &operator[](unsigned) const { __builtin_unreachable(); }
    constexpr const T *begin() const { return nullptr; }
    constexpr const T *end() const { return nullptr; }
};

} // namespace QUARK::Meta

#endif
