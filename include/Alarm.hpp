#pragma once

#include <synchronization/Semaphore.hpp>
#include <utility/collections/Node.hpp>
#include <utility/collections/OrderedList.hpp>

namespace QUARK {

class Alarm : collections::Node<void, Microsecond, true> {
  typedef collections::Node<void, Microsecond, true> Node;
  typedef collections::OrderedList<Node, Spin> Alarms;
  friend Alarms;

public:
  Alarm(Microsecond);
  Alarm(Microsecond, Semaphore &);
  ~Alarm();

  static void handler();

private:
  operator bool();

private:
  static constinit inline Alarms alarms_[Traits<CPU>::Active];

private:
  size_t core_;
  Semaphore internal_;
  Semaphore &handler_;
};

} // namespace QUARK
