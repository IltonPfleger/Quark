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

        if constexpr (DoublyLinked) {
            node->previous = nullptr;
            if (head_) {
                head_->previous = node;
            }
        }

        head_ = node;

        unlock();
    }

    T *remove() {
        lock();

        if (!head_) {
            unlock();
            return nullptr;
        }

        T *node = head_;
        head_   = node->next;

        if constexpr (DoublyLinked) {
            if (head_) {
                head_->previous = nullptr;
            }
            node->previous = nullptr;
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
            }
            node->previous = nullptr;
        } else {
            if (head_ == node) {
                head_ = node->next;
            } else {
                T *current = head_;
                while (current && current->next != node) {
                    current = current->next;
                }
                if (current) {
                    current->next = node->next;
                } else {
                    found = false;
                }
            }
        }

        if (found) node->next = nullptr;

        unlock();

        return found;
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
    static constexpr bool DoublyLinked = !Meta::Same<typename T::Previous, Meta::Empty>::Result;

  private:
    T *head_;
    Meta::IF<!Meta::IsVoid<Lock>::Result, Lock, Meta::Empty>::Result lock_{};
};

} // namespace QUARK::collections
