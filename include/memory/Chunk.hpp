#pragma once

#include <types.hpp>

namespace QUARK {

class Chunk {
public:
  constexpr Chunk() : start_(0), length_(0) {}

  constexpr Chunk(uintptr_t start, size_t length)
      : start_(start), length_(length) {}

  constexpr Chunk(const void *start, size_t length)
      : start_(reinterpret_cast<uintptr_t>(start)), length_(length) {}

  [[nodiscard]]
  constexpr uintptr_t start() const {
    return start_;
  }

  [[nodiscard]]
  constexpr size_t length() const {
    return length_;
  }

  [[nodiscard]]
  constexpr uintptr_t end() const {
    return start() + length();
  }

  template <typename T = unsigned char>
  [[nodiscard]]
  T *data() const {
    return reinterpret_cast<T *>(start());
  }

  [[nodiscard]]
  constexpr bool empty() const {
    return length() == 0;
  }

  [[nodiscard]]
  constexpr bool contains(uintptr_t address) const {
    return (address >= start()) && (address < end());
  }

  [[nodiscard]]
  constexpr bool contains(const Chunk &other) const {
    return start() <= other.start() && end() >= other.end();
  }

  [[nodiscard]]
  constexpr bool overlaps(const Chunk &other) const {
    return start() < other.end() && other.start() < end();
  }

  [[nodiscard]]
  constexpr bool operator==(const Chunk &other) const {
    return start() == other.start() && length() == other.length();
  }

  [[nodiscard]]
  constexpr bool operator!=(const Chunk &other) const {
    return !(*this == other);
  }

private:
  uintptr_t start_;
  size_t length_;
};

} // namespace QUARK
