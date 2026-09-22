#pragma once

#include <synchronization/Mutex.hpp>
#include <utility/collections/UnorderedList.hpp>

namespace QUARK {

template <typename... Args> class Observer;
template <typename... Args> class Observed;

template <typename... Args> class Observed {
  friend class Observer<Args...>;
  using Node = collections::Node<Observer<Args...> *, void, true>;
  using List = collections::UnorderedList<Node>;

public:
  Observed() = default;

  virtual void attach(Observer<Args...> *observer) {
    observers_.insert(&observer->node_);
  }
  virtual void detach(Observer<Args...> *observer) {
    observers_.remove(&observer->node_);
  }

  virtual void notify(Args... args) {
    for (auto *l = observers_.head(); l; l = l->next) {
      l->value->update(args...);
    }
  }

private:
  List observers_;
};

template <typename... Args> class Observer {
  friend class Observed<Args...>;

public:
  Observer() : node_(this) {}

  virtual ~Observer() = default;

  virtual void update(Args... args) = 0;

private:
  typename Observed<Args...>::Node node_;
};

} // namespace QUARK
