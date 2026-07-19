#pragma once

#include <Mutex.hpp>
#include <utility/collections/UnorderedList.hpp>

namespace QUARK {

template <typename... Args> class Observer;
template <typename... Args> class Observed;

template <typename... Args> class Observed {
  public:
    Observed() = default;

    virtual void attach(Observer<Args...> *observer) { observers_.insert(&observer->node_); }
    virtual void detach(Observer<Args...> *observer) { observers_.remove(&observer->node_); }

    virtual void notify(Args... args) {
        for (auto *l = observers_.head(); l; l = l->next) {
            l->value->update(args...);
        }
    }

  private:
    collections::UnorderedList<collections::Node<Observer<Args...> *, void, true>, Mutex> observers_;
};

template <typename... Args> class Observer {
    friend class Observed<Args...>;

  public:
    Observer()
        : node_(this) {}

    virtual ~Observer() = default;

    virtual void update(Args... args) = 0;

  private:
    typename collections::Node<Observer<Args...> *, void, true> node_;
};

} // namespace QUARK
