#pragma once

namespace QUARK::collections {

template <typename T, typename Lock = void> class OrderedList {
  public:
    constexpr OrderedList() = default;

    void insert(T *node) {
        lock();

        if (!this->head_) {
            node->next     = nullptr;
            node->previous = nullptr;
            this->head_    = node;
            this->tail_    = node;
        } else if (node->criterion < this->head_->criterion) {
            node->next            = this->head_;
            node->previous        = nullptr;
            this->head_->previous = node;
            this->head_           = node;
        } else {
            T *current = this->head_;

            while (current->next && current->next->criterion <= node->criterion) {
                current = current->next;
            }

            node->next     = current->next;
            node->previous = current;

            if (current->next) {
                current->next->previous = node;
            } else {
                this->tail_ = node;
            }

            current->next = node;
        }

        unlock();
    }

    T *remove() {
        lock();

        if (!this->head_) {
            unlock();
            return nullptr;
        }

        T *node     = this->head_;
        this->head_ = this->head_->next;

        if (this->head_) {
            this->head_->previous = nullptr;
        } else {
            this->tail_ = nullptr;
        }

        node->next     = nullptr;
        node->previous = nullptr;

        unlock();

        return node;
    }

    bool remove(T *node) {
        lock();

        if (node == this->head_) {
            this->head_ = node->next;
        } else if (node->previous) {
            node->previous->next = node->next;
        } else {
            unlock();
            return false;
        }

        if (node == this->tail_) {
            this->tail_ = node->previous;
        } else if (node->next) {
            node->next->previous = node->previous;
        }

        unlock();

        return true;
    }

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
    T *head_ = nullptr;
    T *tail_ = nullptr;
    Meta::IF<!Meta::IsVoid<Lock>::Result, Lock, Meta::Empty>::Result lock_{};
};

} // namespace QUARK::collections
