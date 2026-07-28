#ifndef __QUARK_UTILITY_COLLECTIONS_MPSC__
#define __QUARK_UTILITY_COLLECTIONS_MPSC__

#include <utility/Atomic.hpp>

namespace QUARK::collections {

template <typename T> class MPSC {
  public:
    constexpr MPSC() {
        stub_.next = nullptr;
        head_ = tail_ = &stub_;
    }

    constexpr ~MPSC() = default;

    void insert(T *node) {
        node->next     = nullptr;
        T *previous    = tail_.exchange(node);
        previous->next = node;
    }

    T *remove() {
        T *tail = tail_;
        T *next = head_->next;

        if (head_ == &stub_) {
            if (!next) return nullptr;
            head_ = next;
            next  = next->next;
        }

        volatile T *node = head_;

        if (next) {
            head_      = next;
            node->next = nullptr;
            return const_cast<T *>(node);
        }

        if (node == tail) {
            insert(&stub_);
            next = node->next;
            if (next) {
                head_      = next;
                node->next = nullptr;
                return const_cast<T *>(node);
            }
        }

        while ((next = node->next) == nullptr)
            ;
        head_      = next;
        node->next = nullptr;
        return const_cast<T *>(node);
    }

  private:
    T stub_;
    T *head_;
    Atomic<T *> tail_;
};

} // namespace QUARK::collections

#endif
