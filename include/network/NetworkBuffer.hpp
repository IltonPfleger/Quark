#ifndef __QUARK_NETWORK_BUFFER__
#define __QUARK_NETWORK_BUFFER__

#include <types.hpp>
#include <utility/Atomic.hpp>
#include <utility/collections/Node.hpp>

namespace QUARK {

class NetworkDevice;

class NetworkBuffer {
  public:
    using Node = collections::Node<NetworkBuffer *>;

    NetworkBuffer(void *start, size_t head, size_t tail, Atomic<uint32_t> *references = nullptr)
        : start_(static_cast<uint8_t *>(start)),
          head_(start_ + head),
          tail_(start_ + tail),
          node_(this),
          references_(references) {}

    NetworkBuffer()
        : NetworkBuffer(nullptr, 0, 0, nullptr) {}

    template <typename T = uint8_t *>
    [[nodiscard]]
    T data(this auto &&self) {
        return reinterpret_cast<T>(self.head_);
    }

    template <typename T = uint8_t *>
    [[nodiscard]]
    T start(this auto &&self) {
        return reinterpret_cast<T>(self.start_);
    }

    [[nodiscard]]
    uint8_t operator[](this auto &&self, size_t i) {
        return self.head_[i];
    }

    [[nodiscard]]
    size_t references() const {
        assert(references_);
        return *references_;
    }

    [[nodiscard]]
    size_t length() const {
        return static_cast<size_t>(tail_ - start_);
    }

    [[nodiscard]]
    size_t offset() const {
        return static_cast<size_t>(head_ - start_);
    }

    [[nodiscard]]
    Node *node() const {
        return const_cast<Node *>(&node_);
    }

    bool advance(size_t bytes) {
        head_ += bytes;
        return true;
    }

    bool rewind(size_t bytes) {
        head_ -= bytes;
        return true;
    }

    bool extend(size_t bytes) {
        tail_ += bytes;
        return true;
    }

    bool shrink(size_t bytes) {
        tail_ -= bytes;
        return true;
    }

    void hold() const {
        assert(references_);
        references_->finc();
    };

  private:
    uint8_t *const start_;

    uint8_t *head_;
    uint8_t *tail_;

    Node node_;

    mutable Atomic<uint32_t> *references_;
};

} // namespace QUARK

#endif
