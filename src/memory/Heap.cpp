#include <memory/Heap.hpp>

extern "C" void *malloc(QUARK::size_t size) {
  using namespace QUARK;

  assert(size);

  void *raw = Memory::alloc(size + sizeof(HeapHeader));

  HeapHeader *header = reinterpret_cast<HeapHeader *>(raw);

  header->size = size + sizeof(HeapHeader);

  return header + 1;
};

extern "C" void free(void *pointer) {
  using namespace QUARK;

  assert(pointer);

  HeapHeader *header = reinterpret_cast<HeapHeader *>(pointer);
  header -= 1;
  Memory::free(header, header->size);
}
