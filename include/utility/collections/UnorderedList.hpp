#pragma once

#include <types.hpp>
#include <utility/Debug.hpp>
#include <utility/collections/Node.hpp>

namespace QUARK::collections {

template <typename T, typename Lock = void> class UnorderedList {
  public:
    constexpr UnorderedList()
        : head_(nullptr) {}

    T *head() { return head_; }

    void insert(T *node) {
        assert(node);

        lock();

        node->next = head_;
        head_      = node;

        unlock();
    }

    T *remove() {
        lock();

        if (!head_) {
            unlock();
            return nullptr;
        }

        T *node    = head_;
        head_      = node->next;
        node->next = nullptr;

        unlock();

        return node;
    }

    bool remove(T *target) {
        assert(target);

        lock();

        T *previous = nullptr;
        T *current  = head_;

        while (current && current != target) {
            previous = current;
            current  = current->next;
        }

        if (!current) {
            unlock();
            return false;
        }

        if (previous) {
            previous->next = current->next;
        } else {
            head_ = current->next;
        }

        current->next = nullptr;

        unlock();

        return true;
    }

  protected:
    void lock() {
        if constexpr (!Meta::IsVoid<Lock>::Result) {
            lock_.acquire();
        }
    }

    void unlock() {
        if constexpr (!Meta::IsVoid<Lock>::Result) {
            lock_.release();
        }
    }

  private:
    T *head_;
    Meta::IF<!Meta::IsVoid<Lock>::Result, Lock, Meta::Empty>::Result lock_{};
};

} // namespace QUARK::collections
