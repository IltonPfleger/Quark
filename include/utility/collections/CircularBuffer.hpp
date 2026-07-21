#ifndef __QUARK_CIRCULAR_BUFFER__
#define __QUARK_CIRCULAR_BUFFER__

namespace QUARK::collections {

template <typename T, size_t Capacity, typename Lock = void> class CircularBuffer {
  public:
    CircularBuffer()
        : head_(0),
          tail_(0),
          length_(0) {}

    bool insert(const T &source) {
        bool response = true;

        lock();

        buffer_[tail_] = source;
        tail_          = (tail_ + 1) % Capacity;

        if (length_ == Capacity) {
            head_    = (head_ + 1) % Capacity;
            response = false;
        } else {
            ++length_;
        }

        unlock();

        return response;
    }

    bool remove(T &destination) {
        lock();

        if (length_ == 0) {
            unlock();
            return false;
        }

        destination = buffer_[head_];
        head_       = (head_ + 1) % Capacity;
        --length_;

        unlock();
        return true;
    }

    void lock() {
        if constexpr (!Meta::Same<Lock, Meta::Empty>::Result) {
            lock_.acquire();
        }
    }

    void unlock() {
        if constexpr (!Meta::Same<Lock, Meta::Empty>::Result) {
            lock_.release();
        }
    }

    T &operator[](size_t index) { return buffer_[index]; }
    const T &operator[](size_t index) const { return buffer_[index]; }

    T *begin() { return &buffer_[0]; }
    const T *begin() const { return &buffer_[0]; }

    T *end() { return &buffer_[Capacity - 1]; }
    const T *end() const { return &buffer_[Capacity - 1]; }

  private:
    T buffer_[Capacity];

    size_t head_;
    size_t tail_;
    size_t length_;

    Meta::IF<!Meta::IsVoid<Lock>::Result, Lock, Meta::Empty>::Result lock_;
};

} // namespace QUARK::collections

#endif
