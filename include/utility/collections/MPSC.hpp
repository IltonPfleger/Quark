#ifndef __QUARK_UTILITY_COLLECTIONS_MPSC__
#define __QUARK_UTILITY_COLLECTIONS_MPSC__

namespace QUARK::collections {

template <typename T, typename Lock = void> class MPSC {
public:
  constexpr MPSC() : stub_(), head_(nullptr), tail_(nullptr) {
    stub_.next = nullptr;
    head_ = tail_ = &stub_;
  }

  MPSC(const MPSC &) = delete;
  MPSC &operator=(const MPSC &) = delete;
  MPSC(MPSC &&other) = delete;
  MPSC &operator=(MPSC &&other) = delete;

  void insert(T *element) {
    element->next = nullptr;
    T *previous = CPU::Atomic::exchange(tail_, element);
    previous->next = element;
    CPU::mbw();
  }

  T *remove() {
    CPU::mbr();
    T *tail = head_;
    T *next = tail->next;

    if (tail == &stub_) {
      if (!next)
        return nullptr;
      head_ = next;
      tail = next;
      next = next->next;
    }

    if (next) {
      head_ = next;
      return tail;
    }

    T *real_tail = tail_;
    if (tail != real_tail) {
      while ((next = tail->next) == nullptr) {
      }
      head_ = next;
      return tail;
    }

    insert(&stub_);

    next = tail->next;
    if (next) {
      head_ = next;
      return tail;
    }

    return nullptr;
  }

private:
  T *head_;
  T *tail_;
  T stub_;
};

} // namespace QUARK::collections

#endif
