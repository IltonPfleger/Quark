#pragma once

#include <utility/collections/UnorderedList.hpp>

namespace QUARK::allocators {

template <size_t MINIMUM, size_t MAXIMUM> class Buddy {
  typedef collections::Node<void> Node;

  static_assert(MINIMUM <= MAXIMUM);
  static_assert(1 << MINIMUM >= sizeof(Node));

  static constexpr size_t Minimum = MINIMUM;
  static constexpr size_t Maximum = MAXIMUM - Minimum;

public:
  constexpr Buddy() : free_() {}

  [[nodiscard]]
  void *remove(size_t size) {
    Node *node = nullptr;
    size_t n = bucket(size);
    size_t i = n;

    for (; i <= Maximum; ++i) {
      node = free_[i].remove();
      if (node)
        break;
    }

    if (!node)
      return nullptr;

    while (i > n) {
      i--;
      uintptr_t b = buddy(reinterpret_cast<uintptr_t>(node), i);
      free_[i].insert(reinterpret_cast<Node *>(b));
    }

    return node;
  }

  void insert(void *pointer, size_t size) {
    uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
    constexpr size_t minimum = 1ULL << MINIMUM;
    constexpr size_t maximum = 1ULL << MAXIMUM;

    while (size >= minimum) {
      size_t current = minimum;

      while ((current << 1) <= size && (current << 1) <= maximum &&
             !(address & ((current << 1) - 1))) {
        current <<= 1;
      }

      insert(address, current);
      address += current;
      size -= current;
    }
  }

private:
  void insert(uintptr_t address, size_t size) {
    size_t n = bucket(size);

    assert(n <= Maximum);
    assert((address & ((1ULL << log2ceil(size)) - 1)) == 0);

    while (n < Maximum) {
      uintptr_t b = buddy(address, n);
      Node *buddy = reinterpret_cast<Node *>(b);

      if (!free_[n].remove(buddy))
        break;

      if (b < address)
        address = b;
      ++n;
    }

    free_[n].insert(reinterpret_cast<Node *>(address));
  }

  constexpr static size_t log2ceil(size_t size) {
    size_t l = 0;
    while ((1ULL << l) < size)
      l++;
    return l;
  }

  static constexpr size_t bucket(size_t size) {
    size_t b = log2ceil(size);
    if (b < Minimum)
      b = Minimum;
    return b - Minimum;
  }

  static constexpr uintptr_t buddy(uintptr_t address, size_t bucket) {
    uintptr_t size = 1ULL << (bucket + Minimum);
    return address ^ size;
  }

private:
  collections::UnorderedList<Node> free_[Maximum + 1];
};

} // namespace QUARK::allocators
