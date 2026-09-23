// #include <memory/Heap.hpp>
//
// extern "C" void *malloc(QUARK::size_t size) {
//   using namespace QUARK;
//   if (size == 0)
//     return nullptr;
//   void *raw = Memory::alloc(size + sizeof(HeapHeader));
//   HeapHeader *header = reinterpret_cast<HeapHeader *>(raw);
//   header->size = size + sizeof(HeapHeader);
//   return header + 1;
// };
//
// extern "C" void free(void *pointer) {
//   using namespace QUARK;
//   if (!pointer)
//     return;
//   auto *header = static_cast<HeapHeader *>(pointer) - 1;
//   Memory::free(header, header->size);
// }
