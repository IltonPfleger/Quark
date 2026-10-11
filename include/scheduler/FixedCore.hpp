#ifndef QUARK_SCHEDULER_FIXED_CORE_HPP
#define QUARK_SCHEDULER_FIXED_CORE_HPP

#include <synchronization/Spin.hpp>
#include <types.hpp>
#include <utility/collections/FIFO.hpp>

namespace QUARK {

class FixedCore {
public:
  enum Rank : uint8_t { IDLE = 0, NORMAL = 1 };
  static constexpr uint32_t ANY = ~0U;
  static constexpr uint32_t NumberOfCores = Traits<CPU>::Active;

  constexpr FixedCore(uint8_t rank = NORMAL, uint32_t cpu = ANY)
      : cpu_(cpu), rank_(rank) {
    if (!__builtin_is_constant_evaluated()) {
      if (cpu_ == ANY) {
        cpu_ = CPU::Atomic::finc(counter_) % NumberOfCores;
      }
      assert(rank_ <= NORMAL);
      assert(cpu_ < NumberOfCores);
    }
  }

  constexpr FixedCore(const FixedCore &) = default;
  constexpr FixedCore &operator=(const FixedCore &) = default;

  constexpr uint32_t cpu() const { return cpu_; }
  constexpr uint8_t rank() const { return rank_; }

  explicit constexpr operator uint8_t() const { return rank_; }

  template <typename Node> struct Collection {
    void insert(const FixedCore &fc, Node *node) {
      queues_[fc.cpu()][fc.rank()].insert(node);
    }

    Node *remove(uint8_t threshold) {
      Node *node = queues_[CPU::id()][NORMAL].remove();
      if (!node && threshold == IDLE) {
        node = queues_[CPU::id()][IDLE].remove();
      }
      return node;
    }

  private:
    collections::FIFO<Node, Spin> queues_[NumberOfCores][2];
  };

private:
  static constinit inline uint32_t counter_ = 0;
  uint32_t cpu_;
  uint8_t rank_;
};

} // namespace QUARK

#endif
