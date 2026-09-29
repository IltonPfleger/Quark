#pragma once

#include <types.hpp>
#include <utility/Debug.hpp>
#include <utility/collections/Node.hpp>

namespace QUARK::collections {

template <typename T, typename LOCK = void> class UnorderedList {

public:
  constexpr UnorderedList() : head_(nullptr), tail_(nullptr), lock_() {}

  T *head() { return head_; }

  void insert(T *node) {
    assert(node);

    lock();

    node->next = nullptr;

    if constexpr (DoublyLinked) {
      node->previous = tail_;
    }

    if (tail_) {
      tail_->next = node;
    } else {
      head_ = node;
    }

    tail_ = node;

    unlock();
  }

  T *remove() {
    lock();

    if (!head_) {
      unlock();
      return nullptr;
    }

    T *node = head_;
    head_ = node->next;

    if constexpr (DoublyLinked) {
      if (head_) {
        head_->previous = nullptr;
      }
      node->previous = nullptr;
    }

    if (!head_) {
      tail_ = nullptr;
    }

    node->next = nullptr;

    unlock();

    return node;
  }

  bool remove(T *node) {
    assert(node);

    lock();

    bool found = true;

    if constexpr (DoublyLinked) {
      if (node->previous) {
        node->previous->next = node->next;
      } else {
        head_ = node->next;
      }

      if (node->next) {
        node->next->previous = node->previous;
      } else {
        tail_ = node->previous;
      }

      node->previous = nullptr;
    } else {
      if (head_ == node) {
        head_ = node->next;

        if (!head_) {
          tail_ = nullptr;
        }
      } else {
        T *current = head_;

        while (current && current->next != node) {
          current = current->next;
        }

        if (current) {
          current->next = node->next;

          if (tail_ == node) {
            tail_ = current;
          }
        } else {
          found = false;
        }
      }
    }

    if (found)
      node->next = nullptr;

    unlock();

    return found;
  }

protected:
  void lock() {
    if constexpr (!Meta::IsVoid<LOCK>::Result) {
      lock_.acquire();
    }
  }

  void unlock() {
    if constexpr (!Meta::IsVoid<LOCK>::Result) {
      lock_.release();
    }
  }

private:
  static constexpr bool DoublyLinked =
      !Meta::Same<typename T::Previous, Meta::Empty>::Result;

private:
  T *head_;
  T *tail_;
  Meta::IF<!Meta::IsVoid<LOCK>::Result, LOCK, Meta::Empty>::Result lock_;
};

} // namespace QUARK::collections
