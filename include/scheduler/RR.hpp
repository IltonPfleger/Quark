#ifndef __QUARK_SCHEDULER_RR__
#define __QUARK_SCHEDULER_RR__

#include <synchronization/Spin.hpp>
#include <types.hpp>
#include <utility/collections/FIFO.hpp>

namespace QUARK {

class RR {

public:
  enum : uint8_t { IDLE = 0, NORMAL = 1 };

  RR(uint8_t rank, ...) : rank_(rank) {}

  operator int() const { return rank_; }

  template <typename Node> struct Collection {
  public:
    void insert(const RR &rr, Node *node) { queues_[rr].insert(node); }

    Node *remove(uint8_t threshold) {
      Node *node = queues_[NORMAL].remove();
      if (!node && threshold == IDLE)
        node = queues_[IDLE].remove();
      return node;
    }

  private:
    collections::FIFO<Node, Spin> queues_[2];
  };

private:
  uint8_t rank_;
};

} // namespace QUARK

#endif
