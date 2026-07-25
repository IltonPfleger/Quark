#ifndef __QUARK_FIFO__
#define __QUARK_FIFO__

namespace QUARK::collections {

template <typename T, typename Lock = void> class FIFO {
  public:
    constexpr FIFO()
        : head_(nullptr),
          tail_(nullptr) {}

    FIFO(const FIFO &)            = delete;
    FIFO &operator=(const FIFO &) = delete;
    FIFO(FIFO &&other)            = delete;
    FIFO &operator=(FIFO &&other) = delete;

    void insert(T *node) {
        assert(!node->next);

        lock();

        node->next = nullptr;

        if (!head_)
            head_ = tail_ = node;
        else
            tail_ = (tail_->next = node);

        unlock();
    }

    T *remove() {
        lock();

        T *node = head_;

        if (node) {
            head_      = node->next;
            node->next = nullptr;
        }

        if (!head_) tail_ = nullptr;

        unlock();

        return node;
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

  protected:
    T *head_;
    T *tail_;
    Meta::IF<!Meta::IsVoid<Lock>::Result, Lock, Meta::Empty>::Result lock_;
};

} // namespace QUARK::collections

#endif
