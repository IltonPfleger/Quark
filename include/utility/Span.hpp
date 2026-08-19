#ifndef __QUARK_UTILITY_SPAN__
#define __QUARK_UTILITY_SPAN__

#include <types.hpp>

namespace QUARK {

template <typename T> class Span {
public:
  using Type = T;
  using Pointer = T *;
  using Reference = T &;
  using ConstReference = const T &;
  using Size = size_t;

public:
  constexpr Span() : _data(nullptr), _length(0) {}

  constexpr Span(T *data, size_t length) : _data(data), _length(length) {}

  template <size_t N>
  constexpr Span(T (&array)[N]) : _data(array), _length(N) {}

public:
  [[nodiscard]]
  constexpr auto data(this auto &&self) {
    return self._data;
  }

  [[nodiscard]]
  constexpr auto &operator[](this auto &&self, size_t i) {
    return self.data()[i];
  }

  [[nodiscard]]
  constexpr operator T *(this auto &&self) {
    return self.data();
  }

  [[nodiscard]]
  constexpr operator const void *() const {
    return static_cast<const void *>(_data);
  }

  [[nodiscard]]
  constexpr operator void *()
    requires(!Meta::Const<T>)
  {
    return static_cast<void *>(_data);
  }

  [[nodiscard]]
  constexpr size_t length(this auto &&self) {
    return self._length;
  }

  [[nodiscard]]
  constexpr bool empty(this auto &&self) {
    return self.length() == 0;
  }

  [[nodiscard]]
  constexpr bool operator==(const Span &other) const {
    if (_length != other._length)
      return false;
    for (size_t i = 0; i < _length; i++)
      if (!(_data[i] == other._data[i]))
        return false;
    return true;
  }

  [[nodiscard]]
  constexpr bool operator!=(const Span &other) const {
    return !(*this == other);
  }

private:
  T *_data;
  size_t _length;
};

} // namespace QUARK

#endif
