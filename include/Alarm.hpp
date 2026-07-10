#pragma once

#include <Semaphore.hpp>
#include <utility/collections/Node.hpp>
#include <utility/collections/OrderedList.hpp>

namespace QUARK {

class Alarm {
    using Node   = collections::Node<Alarm *, Microsecond, true>;
    using Alarms = collections::OrderedList<Node>;

  public:
    Alarm(Microsecond);
    Alarm(Microsecond, Semaphore &);
    ~Alarm();

    static void handler();

  private:
    operator bool();

  private:
    static constinit inline Alarms alarms_[Traits<CPU>::Active];
    static constinit inline Spin locks_[Traits<CPU>::Active];

  private:
    Node node_;
    size_t core_;
    Semaphore local_;
    Semaphore &handler_;
};

} // namespace QUARK
