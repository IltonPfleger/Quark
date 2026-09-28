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

  constexpr Scheduler() : _heads{}, _collection() {}

  Thread *remove(int threshold = Criterion::IDLE) {
    Node *next = _collection.remove(threshold);

    if (next) {
      head(next->value);
      return next->value;
    }

    assert(threshold != Criterion::IDLE);
    return nullptr;
  }

  void insert(Node *node) {
    assert(node);
    _collection.insert(node->criterion, node);
  }

  Thread *current() { return head(); }

private:
  Thread *head() { return _heads[CPU::id()]; }
  void head(Thread *thread) { _heads[CPU::id()] = thread; }

private:
  Thread *volatile _heads[Traits<CPU>::Active];
  typename Criterion::template Collection<Node> _collection;
};

} // namespace QUARK
