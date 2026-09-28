#include <abi/Semaphore.hpp>
#include <abi/Thread.hpp>
#include <memory/Heap.hpp>

using namespace QUARK::ABI;

class Allocator {
public:
  Allocator(size_t max, size_t iterations)
      : max_(max), iterations_(iterations), thread_(worker, this) {}

  static size_t random(size_t max) {
    static size_t seed;
    size_t local = seed++;
    return ((1 << (local % max)) - 1) | !(local % max);
  }

  static void *worker(void *pointer) {
    Allocator *self = reinterpret_cast<Allocator *>(pointer);

    while (self->iterations_--) {
      int size = random(self->max_);
      void *raw = QUARK::Memory::alloc(size);
      auto *buffer = reinterpret_cast<uint8_t *>(raw);
      buffer[0] = buffer[size - 1] = 0b10101010;
      QUARK::free(buffer, size);
    }

    return nullptr;
  }

private:
  size_t max_;
  size_t iterations_;
  Thread thread_;
};

template <size_t MAX, size_t NUMBER, size_t ITERATIONS> class Allocators {
public:
  Allocators()
      : allocators_(
            create(typename Meta::MakeIndexSequence<NUMBER>::Result{})) {}

  template <size_t... Is>
  Meta::Array<NUMBER, Allocator> create(Meta::IndexSequence<Is...>) {
    return {((void)Is, Allocator{MAX, ITERATIONS})...};
  }

private:
  Meta::Array<NUMBER, Allocator> allocators_;
};

int main(int, char *[]) { Allocators<16, 10, 1000> allocators; }
