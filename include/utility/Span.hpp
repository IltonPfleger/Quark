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
  constexpr Span() : data_(nullptr), length_(0) {}

  constexpr Span(T *data, size_t length) : data_(data), length_(length) {}

  template <size_t N>
  constexpr Span(T (&array)[N]) : data_(array), length_(N) {}

public:
  [[nodiscard]]
  constexpr auto data(this auto &&self) {
    return self.data_;
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
    return static_cast<const void *>(data_);
  }

  [[nodiscard]]
  constexpr operator void *()
    requires(!Meta::Const<T>)
  {
    return static_cast<void *>(data_);
  }

  [[nodiscard]]
  constexpr size_t length(this auto &&self) {
    return self.length_;
  }

  [[nodiscard]]
  constexpr bool empty(this auto &&self) {
    return self.length() == 0;
  }

  [[nodiscard]]
  constexpr bool operator==(const Span &other) const {
    if (length_ != other.length_)
      return false;
    for (size_t i = 0; i < length_; i++)
      if (!(data_[i] == other.data_[i]))
        return false;
    return true;
  }

  [[nodiscard]]
  constexpr bool operator!=(const Span &other) const {
    return !(*this == other);
  }

  [[nodiscard]]
  constexpr uintptr_t pointer() {
    return reinterpret_cast<uintptr_t>(data_);
  }

private:
  T *data_;
  size_t length_;
};

} // namespace QUARK

#endif
