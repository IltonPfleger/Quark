#pragma once

#include <Traits.hpp>
#include <architecture/CPU.hpp>
#include <scheduler/FixedCore.hpp>
#include <scheduler/RR.hpp>
#include <utility/Debug.hpp>
#include <utility/collections/Node.hpp>

namespace QUARK {

class Scheduler {
public:
  using Criterion = typename Traits<Scheduler>::Criterion;
  using Node = collections::Node<Thread *, Criterion>;

  constexpr Scheduler() : _collection() {}

  Node *remove(decltype(Criterion::IDLE) threshold) {
    return _collection.remove(threshold);
  }

  void insert(Node *node) {
    assert(node);
    _collection.insert(node->criterion, node);
  }

private:
  typename Criterion::template Collection<Node> _collection;
};

} // namespace QUARK
