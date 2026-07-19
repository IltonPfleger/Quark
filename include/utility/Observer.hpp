#pragma once

#include <Mutex.hpp>
#include <utility/collections/UnorderedList.hpp>

namespace QUARK {

template <typename... Args> class Observer;
template <typename... Args> class Observed;

template <typename... Args> class Observed : collections::UnorderedList<collections::Node<Observer<Args...> *, void, true>, Mutex> {
  public:
    Observed() = default;

    void attach(Observer<Args...> *o) { this->insert(&o->node_); }
    void detach(Observer<Args...> *o) { this->remove(&o->node_); }

    void notify(Args... args) {
        for (auto *l = this->head(); l; l = l->next) {
            l->value->update(args...);
        }
    }
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
