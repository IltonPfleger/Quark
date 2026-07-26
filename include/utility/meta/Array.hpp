#ifndef __QUARK_UTILITY_META_ARRAY__
#define __QUARK_UTILITY_META_ARRAY__

namespace QUARK::Meta {

template <unsigned N, typename T> struct Array {
    constexpr T &operator[](unsigned i) { return m_data[i]; }
    constexpr const T &operator[](unsigned i) const { return m_data[i]; }

    constexpr T *begin() { return m_data; }
    constexpr T *end() { return m_data + N; }

    constexpr const T *begin() const { return m_data; }
    constexpr const T *end() const { return m_data + N; }

  private:
    T m_data[N];
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
